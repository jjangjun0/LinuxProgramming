#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char *fname = "ssu_test.txt";
    int fd;

    printf("First printf: Hello, OSLAB!!\n");

    if ((fd = open(fname, O_RDONLY)) < 0) {
        fprintf(stderr, "open error for %s\n", fname);
        exit(1);
    }

    if (freopen(fname, "w", stdout) != NULL)
        printf("Second printf: Hello, OSLAB!!\n");

    exit(0);
}