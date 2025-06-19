#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERR_GRAMMER 10

char *commandSet[10] = {
	"tree",
	"print",
	"help",
	"exit"
};
void print_tree();
void print_print();
void print_help();
void print_exit();

int main(int argc, char *argv[]) {
	// 등록되지 않은 명령어 인자가 들어오면 help 명령어를 수행하지 않는다.
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], commandSet[0]) == 0)
			continue;
		else if (strcmp(argv[i], commandSet[1]) == 0)
			continue;
		else if (strcmp(argv[i], commandSet[2]) == 0)
			continue;
		else if (strcmp(argv[i], commandSet[3]) == 0)
			continue;
		else {
			fprintf(stderr, "Error: invalid command -- '%s'\n", argv[i]);
			//exit(ERR_GRAMMER); // 이렇게 하면 fork -> { help help }
			argc = 1;
			break;
		}
	}

	// 출력
	printf("Usage:\n");
	if (argc == 1) {
		print_tree();
		print_print();
		print_help();
		print_exit();
		exit(0);
	}
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], commandSet[0]) == 0)
			print_tree();
		else if (strcmp(argv[i], commandSet[1]) == 0)
			print_print();
		else if (strcmp(argv[i], commandSet[2]) == 0)
			print_help();
		else if (strcmp(argv[i], commandSet[3]) == 0)
			print_exit();
	}
	exit(0);
}
void print_tree() {
	printf("  > tree <PATH> [OPTION]... : display the directory structure if <PATH> is a directory\n");
	printf("     -r : display the directory structure recursively if <PATH> is a directory\n");
	printf("     -s : display the directory structure if <PATH> is a directory, including the size of each file\n");
	printf("     -p : display the directory structure if <PATH> is a directory, including the permissions of each directory and file\n");
}
void print_print() {
	printf("  > print <PATH> [OPTION]... : print the contents on the standard output if <PATH> is file\n");
	printf("     -n <line_number> : print only the first <line_number> lines of its contents on the standard output if <PATH> is file\n");
}
void print_help() {
	printf("  > help [COMMAND] : show commands for program\n");
}
void print_exit() {
	printf("  > exit : exit program\n");
}
