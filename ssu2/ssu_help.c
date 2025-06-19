#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERR_GRAMMER 10

char *commandSet[10] = {
	"show",
	"add",
	"modify",
	"remove",
	"help",
	"exit"
};
void print_show();
void print_add();
void print_modify();
void print_remove();
void print_help();
void print_exit();

int main(int argc, char *argv[]) {
	printf("Usage:\n");
	if (argc == 1) {
		print_show();
		print_add();
		print_modify();
		print_remove();
		print_help();
		print_exit();
		exit(0);
	}
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], commandSet[0]) == 0)
			print_show();
		else if (strcmp(argv[i], commandSet[1]) == 0)
			print_add();
		else if (strcmp(argv[i], commandSet[2]) == 0)
			print_modify();
		else if (strcmp(argv[i], commandSet[3]) == 0)
			print_remove();
		else if (strcmp(argv[i], commandSet[4]) == 0)
			print_help();
		else if (strcmp(argv[i], commandSet[5]) == 0)
			print_exit();
		else {
			fprintf(stderr, "%s is not command.\n", argv[i]);
			exit(ERR_GRAMMER);
		}
	}
	printf("\n");
	exit(0);
}
void print_show() {
	printf("  > show\n");
	printf("     <none> : show monitoring deamon process info\n");
}
void print_add() {
	printf("  > add <DIR_PATH> [OPTION]...\n");
	printf("     <none> : add deamon process monitoring the <DIR_PATH> directory\n");
	printf("     -d  <OUTPUT_PATH>  : Specify the output directory <OUTPUT_PATH> where <DIR_PATH> will be arranged\n");
	printf("     -i <TIME_INTERVAL> : Set the time interval for the daemon process to monitor in seconds.\n");
	printf("     -l <MAX_LOG_LINES> : Set the maximum number of log lines the daemon process will record.\n");
	printf("     -x <EXCLUDE_PATH1, EXCLUDE_PATH2, ...> : Exclude all subfiles in the specified directories.\n");
	printf("     -e <EXTENSION1, EXTENSION2, ...> : Specify the file extensions to be organized.\n");
	printf("     -m <M> : Specify the value for the <M> option.\n");
}
void print_modify() {
	printf("  > modify <DIR_PATH> [OPTION]...\n");
	printf("     <none> : modify deamon process config monitoring the <DIR_PATH> directory\n");
	printf("     -d  <OUTPUT_PATH>  : Specify the output directory <OUTPUT_PATH> where <DIR_PATH> will be arranged\n");
	printf("     -i <TIME_INTERVAL> : Set the time interval for the daemon process to monitor in seconds.\n");
	printf("     -l <MAX_LOG_LINES> : Set the maximum number of log lines the daemon process will record.\n");
	printf("     -x <EXCLUDE_PATH1, EXCLUDE_PATH2, ...> : Exclude all subfiles in the specified directories.\n");
	printf("     -e <EXTENSION1, EXTENSION2, ...> : Specify the file extensions to be organized.\n");
	printf("     -m <M> : Specify the value for the <M> option.\n");
}
void print_remove() {
	printf("  > remove <DIR_PATH>\n");
	printf("     <none> : remove deamon process monitoring the <DIR_PATH> directory\n");
}
void print_help() {
	printf("  > help [COMMAND]\n");
}
void print_exit() {
	printf("  > exit\n");
}
