#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    FILE *fp;
    int character;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    if ((fp = fopen(argv[1], "r")) == NULL) {
        fprintf(stderr, "fopen error for %s\n", argv[1]);
        exit(1);
    }

    character = fgetc(fp);
    
    while (!feof(fp)) {
        fputc(character, stdout);

        if (ferror(fp)) {
            fprintf(stderr, "Error detected!!\n");
            clearerr(fp);
        }
        character = fgetc(fp);
    }

    fclose(fp);
    exit(0);
}