#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_FRAGMENT_SIZE 4096

void sigint_handler(int sig) {
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Использование: %s <текст> <количество процессов>\n", argv[0]);
        exit(1);
    }

    int num_processes = atoi(argv[2]);
    int fragment_count = strlen(argv[1]) / num_processes;
    pid_t pids[num_processes];
    int pipes[num_processes][2];

    char fragments[num_processes][MAX_FRAGMENT_SIZE];

    signal(SIGINT, sigint_handler);

    for (int i = 0; i < num_processes; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pids[i] = fork();
        if (pids[i] == 0) {
            // Дочерний процесс
            close(pipes[i][0]); // Закрываем сторону чтения канала
            dup2(pipes[i][1], STDOUT_FILENO);
            close(pipes[i][1]); // Закрываем сторону записи канала
            int fragment_start = i * fragment_count;
            int fragment_size = (i == num_processes - 1) ? strlen(argv[1]) - fragment_start : fragment_count;
            char fragment_size_str[10];
            sprintf(fragment_size_str, "%d", fragment_size);
            char index_str[10];
            sprintf(index_str, "%d", i);
            execl("./encryptor", "encryptor", argv[1] + fragment_start, fragment_size_str, index_str, (char *)NULL);
            exit(0);
        } else if (pids[i] < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < num_processes; i++) {
        waitpid(pids[i], NULL, 0);
        close(pipes[i][1]); // Закрываем сторону записи канала
        char buffer[MAX_FRAGMENT_SIZE];
        int read_bytes = read(pipes[i][0], buffer, MAX_FRAGMENT_SIZE);
        buffer[read_bytes] = '\0';
        int index = atoi(buffer);
        strcpy(fragments[index], buffer + 2);
    }
    printf("Зашифрованный текст: ");
    for (int i = 0; i < num_processes; i++) {
        printf("%s", fragments[i]);
    }
    printf("\n");
    return 0;
}
