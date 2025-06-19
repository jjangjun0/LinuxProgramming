#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	FILE *fp;
	int ch;
	char word[128];
	int index = 0;
	int new_flag = 0; // Is it new world?

	if ((fp = fopen("ssu_words.txt", "r")) == NULL) {
		fprintf(stderr, "fopen error\n");
		exit(1);
	}

	while (!feof(fp)) {
		while ((ch = fgetc(fp)) != EOF &&
				((ch >= 'a' && ch <= 'z')) || (ch >= 'A' && ch <= 'Z')){
			word[index++] = ch;
			new_flag = 1; // flag ON
		}
		word[index] = '\0';

		if (new_flag == 1) {
			fprintf(stdout, "%s\n", word); // Only new word print!
			index = 0;

			new_flag = 0; // flag OFF
		}

		if (ch != EOF) {
			ungetc(ch, fp);
			ch = fgetc(fp);

			if (ch != ' ' && ch != '\n')
				fprintf(stdout, "Separator => %c\n", ch);
		}
	}

	fclose(fp);

	exit(0);
}
