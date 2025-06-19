#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int character;

    while ((character = getc(stdin)) != EOF) {
        if (putc(character, stdout) == EOF) {
            fprintf(stderr, "standard output error\n");
            exit(1);
        }
    }

    if (ferror(stdin)) {
        fprintf(stderr, "standard input error\n");
        exit(1);
    }

    exit(0);
}