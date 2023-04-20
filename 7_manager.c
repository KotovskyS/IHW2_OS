#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

typedef struct {
    int text_ready;
    int text_processed;
    char input[256];
    char output[256];
} SharedMemory;

sem_t *sem_text_ready;
sem_t *sem_text_processed;

void cleanup() {
    sem_unlink("/text_ready");
    sem_unlink("/text_processed");
    shm_unlink("/shared_memory");
}

void sigint_handler(int sig) {
    cleanup();
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    int num_processes = 4;
    if (argc > 1) {
        num_processes = atoi(argv[1]);
    }

    atexit(cleanup);
    signal(SIGINT, sigint_handler);
    sem_text_ready = sem_open("/text_ready", O_CREAT | O_EXCL, 0600, 0);
    sem_text_processed = sem_open("/text_processed", O_CREAT | O_EXCL, 0600, 0);
    int fd = shm_open("/shared_memory", O_CREAT | O_EXCL | O_RDWR, 0600);
    ftruncate(fd, sizeof(SharedMemory));
    SharedMemory *shared_memory = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    // Запуск процессов-шифровальщиков
    for (int i = 0; i < num_processes; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            execl("./encryptor", "./encryptor", NULL);
            perror("execl");
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    // Менеджер передает текст и собирает результаты
    while (fgets(shared_memory->input, 256, stdin) != NULL) {
        sem_post(sem_text_ready);
        sem_wait(sem_text_processed);
        printf("Encrypted: %s\n", shared_memory->output);
    }
    return 0;
}
