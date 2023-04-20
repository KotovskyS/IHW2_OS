#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <time.h>

#define SHARED_MEM_SIZE 4096

typedef struct {
    char input[SHARED_MEM_SIZE];
    char output[SHARED_MEM_SIZE];
    int fragment_count;
    int num_processes;
} shared_memory;

shared_memory *shared_mem_ptr;
int semid;

void encrypt(char *input, char *output, int fragment_size) {
    for (int i = 0; i < fragment_size; i++) {
        output[i] = input[i] + 3;
    }
}

int main() {
    key_t key = ftok(".", 'a');
    int shmid = shmget(key, sizeof(shared_memory), 0666);
    shared_mem_ptr = (shared_memory *)shmat(shmid, NULL, 0);
    key_t sem_key = ftok(".", 'b');
    semid = semget(sem_key, 1, 0666);
    struct sembuf sem_op;
    sem_op.sem_num = 0;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;
    semop(semid, &sem_op, 1);
    int process_id = --shared_mem_ptr->num_processes;
    int fragment_start = process_id * shared_mem_ptr->fragment_count;
    int fragment_size = (process_id == shared_mem_ptr->num_processes - 1) ?
                        strlen(shared_mem_ptr->input) - fragment_start :
                        shared_mem_ptr->fragment_count;
    encrypt(shared_mem_ptr->input + fragment_start,
            shared_mem_ptr->output + fragment_start,
            fragment_size);
    sem_op.sem_op = 1;
    semop(semid, &sem_op, 1); // Посылаем сигнал
    shmdt(shared_mem_ptr);
    return 0;
}
