#include <stdio.h>
#include <stdlib.h>

#define NAME_SIZE 256

int main(void)
{
	FILE *fp1, *fp2, *fp3;
	char buf1[NAME_SIZE], buf2[NAME_SIZE];
	char fname1[NAME_SIZE];
	char fname2[NAME_SIZE];
	char temp[] = "temp_merge.txt";
	int line_number = 1;

	printf("Enter your first file name: ");
	scanf("%s", fname1);
	printf("Enter your second file name: ");
	scanf("%s", fname2);

	if ((fp1 = fopen(fname1, "r")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", fname1);
		exit(1);
	}
	if ((fp2 = fopen(fname2, "r")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", fname2);
		exit(1);
	}
	if ((fp3 = fopen(temp, "w")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", temp);
		exit(1);
	}

	int flag1, flag2;
	while (1) {
		flag1 = flag2 = 0;

		if (fgets(buf1, sizeof(buf1), fp1) != NULL) {
			fprintf(fp3, "%d: %s", line_number, buf1);
			line_number++;
		}
		else
			flag1 = 1;

		if (fgets(buf2, sizeof(buf2), fp2) != NULL) {
			fprintf(fp3, "%d: %s", line_number, buf2);
			line_number++;
		}
		else
			flag2 = 1;

		if (flag1 == 1 && flag2 == 1)
			break;
	}


	if (remove(fname1) < 0) {
		fprintf(stderr, "remove error for %s\n", fname1);
		exit(1);
	}
	if (remove(fname2) < 0) {
		fprintf(stderr, "remove error for %s\n", fname2);
		exit(1);
	}
	if (rename(temp, fname1) < 0) {
		fprintf(stderr, "rename error for %s\n", fname1);
		exit(1);
	}

	fclose(fp1);
	fclose(fp2);
	fclose(fp3);

	exit(0);
}
