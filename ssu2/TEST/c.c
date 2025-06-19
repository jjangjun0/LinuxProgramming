#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>


int main(void)
{
	int x = 10;
	printf("%d\n", x);
	printf("%d\n", x+x);
	printf("modify");

	exit(0);
}
