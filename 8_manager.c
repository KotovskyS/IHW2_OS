#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>

#define SHARED_MEM_SIZE 4096

typedef struct {
    char input[SHARED_MEM_SIZE];
    char output[SHARED_MEM_SIZE];
    int fragment_count;
    int num_processes;
} shared_memory;

int shmid;
shared_memory *shared_mem_ptr;
int semid;

void cleanup() {
    shmdt(shared_mem_ptr);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
}

void sigint_handler(int sig) {
    cleanup();
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <text> <number_of_processes>\n", argv[0]);
        exit(1);
    }
    int num_processes = atoi(argv[2]);
    key_t key = ftok(".", 'a');
    shmid = shmget(key, sizeof(shared_memory), IPC_CREAT | 0666);
    shared_mem_ptr = (shared_memory *)shmat(shmid, NULL, 0);
    key_t sem_key = ftok(".", 'b');
    semid = semget(sem_key, 1, IPC_CREAT | 0666);
    semctl(semid, 0, SETVAL, num_processes);
    signal(SIGINT, sigint_handler);
    strcpy(shared_mem_ptr->input, argv[1]);
    shared_mem_ptr->fragment_count = strlen(argv[1]) / num_processes;
    shared_mem_ptr->num_processes = num_processes;

    for (int i = 0; i < num_processes; i++) {
        if (fork() == 0) {
            execl("./encryptor", "./encryptor", NULL);
        }
    }

    for (int i = 0; i < num_processes; i++) {
        wait(NULL);
    }
    printf("Зашифрованный текст: %s\n", shared_mem_ptr->output);
    cleanup();
    return 0;
}
