#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024

int main(void)
{
    char *fname = "ssu_test.dat";
    char name[BUFFER_SIZE];
    FILE *fp;
    int age;

    fp = fopen(fname, "r");
    fscanf(fp, "%s %d", name, &age);
    fclose(fp);

    fp = fopen(fname, "w");
    fprintf(fp, "%s is %d years old\n", name, age);
    fclose(fp);

    exit(0);
}