#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

int filter(const struct dirent *entry) {
	if (strcmp(entry->d_name, ".") == 0 ||
			strcmp(entry->d_name, "..") == 0)
		return 0;

	return 1;
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s <directory path>\n", argv[0]);
		exit(1);
	}

	struct dirent **namelist;
	int num;
	int i;
	char *str;

	// num = scandir(argv[1], &namelist, NULL, alphasort);
	if ((num = scandir(argv[1], &namelist, filter, alphasort)) < 0) {
		fprintf(stderr, "scandir error for %s\n", argv[1]);
		exit(1);
	}
	
	// namelist 순회
	for (i = 0; i < num; i++) {
		if (namelist[i]->d_type == DT_REG)
			str = "regular file";
		else if (namelist[i]->d_type == DT_DIR)
			str = "directory";
		else if (namelist[i]->d_type == DT_LNK)
			str = "symbolic link";
		else if (namelist[i]->d_type == DT_UNKNOWN)
			str = "unknown";

		printf("<%s>: %s\n", namelist[i]->d_name, str);
	}

	// namelist 해제
	for (i = 0; i < num; i++)
		free(namelist[i]);
	free(namelist);

	exit(0);
}
