#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "argus.h"

#define MAX_SIZE 1024


int isExecute(char *str) {
	if (strcmp(str, "-i") == 0) return 1;
	if (strcmp(str, "-m") == 0) return 2;
	if (strcmp(str, "-e") == 0) return 3;
	if (strcmp(str, "-l") == 0) return 4;
	if (strcmp(str, "-t") == 0) return 5;
	if (strcmp(str, "-r") == 0) return 6;
	if (strcmp(str, "-h") == 0) return 7;
	if (strcmp(str, "-o") == 0) return 8;
	return 0;
}


int main(int argc, char *argv[]) {
	int bytes_read;
	char buffer[MAX_SIZE];
	char str[MAX_SIZE];
	char tmp[MAX_SIZE * 5];
	int b = 0;
	int fifo_in;



	fifo_in = open("fifout", O_RDONLY);
	int fifo_fd = open("fifo", O_WRONLY);
	if (fifo_fd == -1) {
		perror("open");
		return -1;
	}

	int i;
	int k = 0;
	for (int j = 1; j < argc; j++) {
		for (i = 0; argv[j][i]; i++, k++)
			buffer[k] = argv[j][i];
		buffer[k] = ' ';
		k++;
	}
	buffer[k - 1] = '\0';

	if (argc < 2) {
		printf("argus $ ");
		fflush(stdout);
		int flag = 1;
		while ((bytes_read = read(0, str, MAX_SIZE)) > 0 && flag) {
			b = 1;
			str[bytes_read - 1] = '\0';
			if (strcmp(str, "quit") == 0) {
				break;
			}

			else if (bytes_read > 1) {
				write(fifo_fd, str, bytes_read);
				int reads = read(fifo_in, &tmp, MAX_SIZE * 10);
				write(1, &tmp, reads);
				tmp[0] = '\0';
			}
			if (b) {
				printf("\nargus $ ");
				fflush(stdout);
			}
		}
	}

	else if (isExecute(argv[1]) > 0) {
		write(fifo_fd, buffer, sizeof(char)*k);
		int reads = read(fifo_in, &tmp, MAX_SIZE * 10);
		write(1, &tmp, reads);
		tmp[0] = '\0';
	}
	close(fifo_fd);
	close(fifo_in);
	return 0;
}