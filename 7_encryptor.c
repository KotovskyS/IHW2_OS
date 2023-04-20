#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
    int text_ready;
    int text_processed;
    char input[256];
    char output[256];
} SharedMemory;

sem_t *sem_text_ready;
sem_t *sem_text_processed;

// Функция шифрования
void encrypt(char *text, int key) {
    for (int i = 0; text[i]; i++) {
        text[i] = (text[i] + key) % 256;
    }
}

int main() {
    sem_text_ready = sem_open("/text_ready", 0);
    sem_text_processed = sem_open("/text_processed", 0);
    int fd = shm_open("/shared_memory", O_RDWR, 0);
    SharedMemory *shared_memory = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    while (1) {
        sem_wait(sem_text_ready);
        encrypt(shared_memory->input, 3);
        for (int i = 0; shared_memory->input[i]; i++) {
            shared_memory->output[i] = shared_memory->input[i];
        }
        sem_post(sem_text_processed);
    }
    return 0;
}
