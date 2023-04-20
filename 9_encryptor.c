#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void encrypt(char *input, char *output, int fragment_size) {
    for (int i = 0; i < fragment_size; i++) {
        output[i] = input[i] + 3;
    }
    output[fragment_size] = '\0';
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Использование: %s <фрагмент> <размер фрагмента> <индекс фрагмента>\n", argv[0]);
        exit(1);
    }

    int fragment_size = atoi(argv[2]);
    char encrypted_text[fragment_size + 1];

    encrypt(argv[1], encrypted_text, fragment_size);

    printf("%s:%s", argv[3], encrypted_text);

    return 0;
}
