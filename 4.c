#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>

#define SHARED_MEM_SIZE 4096
#define SEM_WRITE "/sem_write"
#define SEM_READ "/sem_read"

typedef struct {
    char input[SHARED_MEM_SIZE];
    char output[SHARED_MEM_SIZE];
    int fragment_count;
    int num_processes;
} shared_memory;

static shared_memory *shared_mem_ptr;
static sem_t *sem_write_ptr;
static sem_t *sem_read_ptr;

void cleanup() {
    munmap(shared_mem_ptr, sizeof(shared_memory));
    sem_close(sem_write_ptr);
    sem_close(sem_read_ptr);
    sem_unlink(SEM_WRITE);
    sem_unlink(SEM_READ);
}

void sigint_handler(int sig) {
    cleanup();
    exit(0);
}

void encrypt(char *input, char *output, int fragment_size) {
    // Простое шифрование - смещение на 3 позиции по аски табличке.
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
    sem_write_ptr = sem_open(SEM_WRITE, O_CREAT, 0644, num_processes);
    sem_read_ptr = sem_open(SEM_READ, O_CREAT, 0644, 0);
    int fd = shm_open("/my_shm", O_CREAT | O_RDWR, 0644);
    ftruncate(fd, sizeof(shared_memory));
    shared_mem_ptr = mmap(NULL, sizeof(shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    strcpy(shared_mem_ptr->input, argv[1]);
    shared_mem_ptr->fragment_count = strlen(argv[1]) / num_processes;
    shared_mem_ptr->num_processes = num_processes;

    signal(SIGINT, sigint_handler);

    for (int i = 0; i < num_processes; i++) {
        if (fork() == 0) { // Дочерний процесс
            srand(time(NULL) ^ (getpid() << 16));
            int random_delay = rand() % 5;
            sleep(random_delay); // Имитация случайной задержки
            sem_wait(sem_write_ptr);
            int fragment_start = i * shared_mem_ptr->fragment_count;
            int fragment_size = (i == num_processes - 1) ?
                                strlen(shared_mem_ptr->input) - fragment_start :
                                shared_mem_ptr->fragment_count;
            encrypt(shared_mem_ptr->input + fragment_start,
                    shared_mem_ptr->output + fragment_start,
                    fragment_size);
            sem_post(sem_read_ptr);
            exit(0);
        }
    }

    for (int i = 0; i < num_processes; i++) {
        sem_wait(sem_read_ptr);
		}
    printf("Зашифрованный текст: %s\n", shared_mem_ptr->output);
    for (int i = 0; i < num_processes; i++) {
        sem_post(sem_write_ptr);
    }
    // Ожидание завершения всех дочерних процессов
    for (int i = 0; i < num_processes; i++) {
        wait(NULL);
    }
    cleanup();
    return 0;
}