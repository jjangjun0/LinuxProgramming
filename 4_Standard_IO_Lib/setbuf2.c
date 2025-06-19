#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024

int main(void)
{
    char buf[BUFFER_SIZE];
    int a, b;
    int i;

    setbuf(stdin, buf);
    scanf("%d %d", &a, &b);

    for (i = 0; buf[i] != '\n'; i++)
        putchar(buf[i]);
    putchar('\n');
    exit(0);
}