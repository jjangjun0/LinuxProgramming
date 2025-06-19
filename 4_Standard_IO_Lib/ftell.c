#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char *fname = "ssu_test.txt";
    FILE *fp;
    long fsize;

    if ((fp = fopen(fname, "r")) == NULL) {
        fprintf(stderr, "fopen error for %s\n", fname);
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);

    printf("The size of <%s> is %ld bytes\n", fname, fsize);
    exit(0);
}