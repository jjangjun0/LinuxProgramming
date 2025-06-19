#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(void)
{
    char buf[BUFFER_SIZE];

    while (fgets(buf, BUFFER_SIZE, stdin) != NULL) {
        if (fputs(buf, stdout) == EOF) {
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