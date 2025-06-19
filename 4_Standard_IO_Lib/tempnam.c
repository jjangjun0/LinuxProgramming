#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    char *arg_directory = NULL;
    char *arg_prefix = NULL;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <directory> <prefix>\n", argv[0]);
        exit(1);
    }

    arg_directory = argv[1][0] == '-' ? NULL : argv[1];
    arg_prefix = argv[2][0] == '-' ? NULL : argv[2];

    printf("created : %s\n", tempnam(arg_directory, arg_prefix));

    exit(0);
}