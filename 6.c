#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <time.h>

#define SHARED_MEM_SIZE 4096

typedef struct {
    char input[SHARED_MEM_SIZE];
    char output[SHARED_MEM_SIZE];
    int fragment_count;
    int num_processes;
} shared_memory;

static int shm_id;
static int sem_id;
static shared_memory *shared_mem_ptr;

void cleanup() {
    shmdt(shared_mem_ptr);
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);
}

void sigint_handler(int sig) {
    cleanup();
    exit(0);
}

void encrypt(char *input, char *output, int fragment_size) {
    for (int i = 0; i < fragment_size; i++) {
        output[i] = input[i] + 3;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Использование: %s <текст> <количество процессов>\n", argv[0]);
        exit(1);
    }

    int num_processes = atoi(argv[2]);
    key_t shm_key = ftok("shmfile", 65);
    shm_id = shmget(shm_key, sizeof(shared_memory), 0644 | IPC_CREAT);
    shared_mem_ptr = (shared_memory *) shmat(shm_id, NULL, 0);
    key_t sem_key = ftok("semfile", 75);
    sem_id = semget(sem_key, 2, 0666 | IPC_CREAT);
    semctl(sem_id, 0, SETVAL, num_processes);
    semctl(sem_id, 1, SETVAL, 0);
    signal(SIGINT, sigint_handler);
    strcpy(shared_mem_ptr->input, argv[1]);
    shared_mem_ptr->fragment_count = strlen(argv[1]) / num_processes;
    shared_mem_ptr->num_processes = num_processes;
    struct sembuf sem_op;

    for (int i = 0; i < num_processes; i++) {
        if (fork() == 0) { // Дочерний процесс
            srand(time(NULL) ^ (getpid() << 16));
            int random_delay = rand() % 5;
            sleep(random_delay); // Имитация случайной задержки
            sem_op.sem_num = 0;
            sem_op.sem_op = -1;
            sem_op.sem_flg = 0;
            semop(sem_id, &sem_op, 1);

            int fragment_start = i * shared_mem_ptr->fragment_count;
            int fragment_size = (i == num_processes - 1) ?
                                strlen(shared_mem_ptr->input) - fragment_start :
                                shared_mem_ptr->fragment_count;

            encrypt(shared_mem_ptr->input + fragment_start,
                    shared_mem_ptr->output + fragment_start,
                    fragment_size);
            sem_op.sem_num = 1;
            sem_op.sem_op = 1;
            sem_op.sem_flg = 0;
            semop(sem_id, &sem_op, 1);

            exit(0);
        }
    }

    for (int i = 0; i < num_processes; i++) {
        sem_op.sem_num = 1;
        sem_op.sem_op = -1;
        sem_op.sem_flg = 0;
        semop(sem_id, &sem_op, 1);
    }

    printf("Зашифрованный текст: %s\n", shared_mem_ptr->output);
    for (int i = 0; i < num_processes; i++) {
        sem_op.sem_num = 0;
        sem_op.sem_op = 1;
        sem_op.sem_flg = 0;
        semop(sem_id, &sem_op, 1);
    }
    // Ожидание завершения всех дочерних процессов
    for (int i = 0; i < num_processes; i++) {
        wait(NULL);
    }
    cleanup();
    return 0;
}