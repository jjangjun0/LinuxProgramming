#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char *fname = "ssu_test.txt";
    FILE *fp;
    long position;
    int character;

    if ((fp = fopen(fname, "r")) == NULL) {
        fprintf(stderr, "fopen error for %s\n", fname);
        exit(1);
    }

    printf("Input number >> ");
    scanf("%ld", &position);

    fseek(fp, position - 1, SEEK_SET);
    character = getc(fp);

    printf("%ldth character => %c\n", position, character);
    exit(0);
}