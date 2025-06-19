#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024

int main(void)
{
    char buf[BUFFER_SIZE];
    char name[L_tmpnam];
    FILE *fp;

    printf("Temporary filename <<%s>>\n", tmpnam(name));

    if ((fp = tmpfile()) == NULL) {
        fprintf(stderr, "tmpfile create error!!\n");
        exit(1);
    }

    fputs("create tmpfile success!!\n", fp);
    rewind(fp);
    fgets(buf, sizeof(buf), fp);
    puts(buf);
    exit(0);
}