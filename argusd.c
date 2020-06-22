#include "aux.h"
#include "argus.h"

#define MAX_SIZE 1024

int MAX_INATIVIDADE = 0;
int MAX_EXECUCAO = 0;

Queue* hist;
Queue* executing;
Queue* finished;

int logs_fd;
int idx_fd;

int *pids;
int num_pids = 0, flag = 0;

void sigint_handler(int signum) {
	flag = 1;
}

void timeout_handler_pipe(int signum) {
	for (int i = 0; i < num_pids; i++)
		kill(pids[i], SIGTERM);
	_exit(2);
}

void timeout_handler(int signum) {
	for (int i = 0; i < num_pids; i++)
		kill(pids[i], SIGTERM);
	_exit(1);
}

void timeout_handler_pipe_aux(int signum) {
	for (int i = 0; i < num_pids; i++)
		kill(pids[i], SIGUSR1);
	_exit(1);
}

void sigchld_handler(int signum) {
	int status;
	int cpid = wait(&status);
	if (cpid > 0) {
		Lista temp;
		temp = removeFromId(&executing->lista, cpid);
		executing->total--;
		addNode(&finished->lista, temp->id, temp->order, WEXITSTATUS(status), temp->comando);
		finished->total++;
		int pos = lseek(logs_fd, 0, SEEK_END);
		lseek(idx_fd, 0, SEEK_END);
		write(idx_fd, &pos, sizeof(pos));
		free(temp->comando);
		free(temp);
	}
}


//0 -> concluida
//1 -> execution timeout
//2 -> execution timeout pipe
//3 -> terminated
void sigtermallchildren_handler(int signum) {
	for (int i = 0; i < num_pids; i++)
		kill(pids[i], SIGKILL);
	exit(3);
}

void sigtermallchildren_handler2(int signum) {
	for (int i = 0; i < num_pids; i++)
		kill(pids[i], SIGUSR1);
	exit(3);
}

int mysystem(char *command, int logs_fd, int idx_fd) {
	pid_t fork_ret;
	char *exec_args[20];
	char *string;
	int i = 0;
	pids = malloc(sizeof(int));
	num_pids = 1;
	string = strtok(command, " ");

	while (string != NULL) {
		exec_args[i] = string;
		string = strtok(NULL, " ");
		i++;
	}

	exec_args[i] = NULL;
	if (signal(SIGCHLD, SIG_DFL) == SIG_ERR) {
		perror("signal");
		exit(-1);
	}
	if (signal(SIGALRM, timeout_handler) == SIG_ERR) {
		perror("signal");
		exit(-1);
	}
	if (signal(SIGUSR1, sigtermallchildren_handler) == SIG_ERR) {
		perror("signal");
		exit(-1);
	}
	alarm(MAX_EXECUCAO);
	fork_ret = fork();
	if (fork_ret == -1) {
		perror("fork");
		exit(-1);
	}
	else if (fork_ret > 0) {
		pids[0] = fork_ret;
	}
	else if (fork_ret == 0) {
		dup2(logs_fd, 1);
		execvp(exec_args[0], exec_args);
	}
	wait(NULL);
	alarm(0);
	_exit(0);
}

int exec_command(char* command) {
	char *exec_args[20];
	char *string;
	int exec_ret = 0;
	int i = 0;

	string = strtok(command, " ");
	while (string != NULL) {
		exec_args[i] = string;
		string = strtok(NULL, " ");
		i++;
	}

	exec_args[i] = NULL;
	exec_ret = execvp(exec_args[0], exec_args);
	return exec_ret;
}

int parser(char **command, char *argv) {
	char *string;
	int r = 0;
	string = strtok(argv, "|");
	command[r++] = string;
	while (string) {
		if ((string = strtok(NULL, "|")) == NULL) break;
		command[r++] = string;
	}
	return r;
}

void mypiping(char *argv, int logs_fd, int idx_fd) {
	int num_args;
	int i, pid;
	char *command[MAX_SIZE];
	num_args = parser(command, argv);
	int p[num_args][2];
	num_pids = 0;
	double left;

	if (signal(SIGCHLD, SIG_DFL) == SIG_ERR) {
		perror("signal");
		exit(-1);
	}
	if (signal(SIGUSR1, sigtermallchildren_handler2) == SIG_ERR) {
		perror("signal");
		exit(-1);
	}
	if (signal(SIGALRM, timeout_handler_pipe_aux) == SIG_ERR) {
		perror("signal");
		exit(-1);
	}
	alarm(MAX_EXECUCAO);
	int filho = fork();
	if (filho == -1) {
		perror("fork");
		exit(-1);
	}
	else if (filho > 0) {
		pids = malloc(sizeof(int));
		pids[num_pids++] = filho;
		int status;
		wait(&status);
		alarm(0);
		_exit(WEXITSTATUS(status));
	}
	else if (filho == 0) {
		if (signal(SIGUSR1, sigtermallchildren_handler) == SIG_ERR) {
			perror("signal");
			exit(-1);
		}
		if (signal(SIGALRM, timeout_handler_pipe) == SIG_ERR) {
			perror("signal");
			exit(-1);
		}
		pids = malloc(sizeof(pids) * num_args);
		for (i = 0; i < num_args; i++) {
			if (i == 0) {
				if (pipe(p[i]) == -1) {
					perror("pipe");
					exit(-1);
				}
				switch (pid = fork()) {
				case -1:
					perror("fork");
					exit(-1);
				case 0:
					close(p[i][0]);
					dup2(p[i][1], 1);
					close(p[i][1]);
					exec_command(command[i]);
					_exit(-1);
				default:
					alarm(MAX_INATIVIDADE);
					pids[num_pids++] = pid;
					close(p[i][1]);
					wait(NULL);
				}
			}
			else if (i == num_args - 1) {
				switch ((pid = fork())) {
				case -1:
					perror("fork");
					exit(-1);
				case 0:
					dup2(p[i - 1][0], 0);
					close(p[i - 1][0]);
					dup2(logs_fd, 1);
					exec_command(command[i]);
					_exit(-1);
				default:
					alarm(0);
					close(p[i - 1][0]);
					pids[num_pids++] = pid;
					wait(NULL);
					_exit(0);
				}
			}

			else {
				if (pipe(p[i]) == -1) {
					perror("pipe");
					exit(-1);
				}
				switch ((pid = fork())) {
				case -1:
					perror("fork");
					exit(-1);
				case 0:
					close(p[i][0]);
					dup2(p[i][1], 1);
					close(p[i][1]);

					dup2(p[i - 1][0], 0);
					close(p[i - 1][0]);
					exec_command(command[i]);
					_exit(0);
				default:
					left = alarm(0);
					alarm(left);
					close(p[i][1]);
					close(p[i - 1][0]);
					pids[num_pids++] = pid;
					wait(NULL);
				}
			}
		}
	}
}

int hasSpace(char * s) {
	for (int i = 0; s[i]; i++)
		if (s[i] == ' ') return 1;
	return 0;
}


int main() {
	char buf[MAX_SIZE];
	int fifo_fd, j, fifo_out;
	logs_fd = open("log", O_CREAT | O_RDWR | O_TRUNC, 0666);
	idx_fd = open("log.idx", O_CREAT | O_RDWR | O_TRUNC, 0666);
	int forks;
	fifo_out = open("fifout", O_WRONLY);
	const char *texto;

	hist = novaQueue();
	executing = novaQueue();
	finished = novaQueue();

	while ((fifo_fd = open("fifo", O_RDONLY)) > 0 && !flag) {
		int bytes_read = 0;
		char *buffer, *command, *exec;
		bytes_read = read(fifo_fd, &buf, MAX_SIZE);
		buffer = strdup(buf);
		close(fifo_fd);
		if (strcmp(buf, "") != 0 && bytes_read > 0) {
			if (hasSpace(buf)) {
				buffer = strtok(buf, " ");
				command = buffer;
				buffer = strtok(NULL, "\0");
				exec = buffer;
			}
			else {
				command = buf;
			}
			switch (chooseExecute(command)) {
			case 1:
				MAX_INATIVIDADE = atoi(exec);
				write(fifo_out, "", 1);
				break;
			case 2:
				MAX_EXECUCAO = atoi(exec);
				write(fifo_out, "", 1);
				break;
			case 3:
				if ((exec[0] == 39 && exec[strlen(exec) - 1] == 39) || command[0] == '-') {
					if (command[0] != '-') {
						for (j = 1; exec[j]; j++) {
							exec[j - 1] = exec[j];
						}
						exec[j - 2] = '\0';
					}
					forks = fork();
					if (forks == -1) {
						perror("fork");
						exit(-1);
					}
					else if (forks == 0) {
						if (!detectPiping(exec)) mysystem(exec, logs_fd, idx_fd);
						else mypiping(exec, logs_fd, idx_fd);
						_exit(0);
					}
					else {
						addNode(&executing->lista, forks, hist->total + 1, -1, exec);
						executing->total++;
						if (signal(SIGCHLD, sigchld_handler) == SIG_ERR) {
							perror("signal");
							exit(-1);
						}
						addNode(&hist->lista, forks, hist->total + 1, -1, exec);
						hist->total++;
						const char * t3 = geraMensagem(hist->total);
						write(fifo_out, t3, strlen(t3));
						//free((char*)t3);
					}
				}
				else {
					write(fifo_out, "Comando para execução inválido\n", 35);
				}
				break;
			case 4:
				if (executing -> total != 0) {
					const char *t4 = imprimeLista(&executing->lista);
					if (strlen(t4) == 0) write(fifo_out, "", 1);
					else {
						write(fifo_out, t4, strlen(t4));
						//free((char*)t4);
					}
				}
				else write(fifo_out, "", 1);
				break;
			case 5:
				if (atoi(exec) > 0 && atoi(exec) <= hist->total) {
					kill(getIdIndex(hist->lista, atoi(exec) - 1), SIGUSR1);
				}
				write(fifo_out, "", 1);
				break;
			case 6:
				if (finished -> total != 0) {
					const char *t6 = imprimeFinished(&finished->lista);
					if (strlen(t6) == 0) write(fifo_out, "", 1);
					else {
						write(fifo_out, t6, strlen(t6));
						//free((char*)t6);
					}
				}
				else write(fifo_out, "", 1);
				break;
			case 7:
				if (command[0] != '-') texto = geraAjuda1();
				else texto = geraAjuda2();
				write(fifo_out, texto, strlen(texto));
				//free((char*)texto);
				break;
			case 8:
				if (getTipoFromOrder(finished->lista, atoi(exec)) == 0) {
					int pos = getPosFromOrder(finished->lista, atoi(exec));
					if (pos == 0) {
						lseek(idx_fd, 0, SEEK_SET);
						int next;
						read(idx_fd, &next, sizeof(int));
						char res[next + 1];
						lseek(logs_fd, 0, SEEK_SET);
						read(logs_fd, res, next * sizeof(char));
						res[next] = '\0';
						if (strlen(res) > 0)write(fifo_out, res, strlen(res));
						else write(fifo_out, "", 1);
					}
					else {
						lseek(idx_fd, (pos - 1)*sizeof(int), SEEK_SET);
						int inicio, fim;
						read(idx_fd, &inicio, sizeof(int));
						read(idx_fd, &fim, sizeof(int));
						lseek(logs_fd, inicio, SEEK_SET);
						char res[fim - inicio + 1];
						read(logs_fd, res, fim - inicio);
						if (fim - inicio > 0)write(fifo_out, res, fim - inicio);
						else write(fifo_out, "", 1);
					}
					lseek(logs_fd, 0, SEEK_END);
				}
				else {write(fifo_out, "", 1);}
				break;
			default:
				write(fifo_out, "", 1);
				break;
			}
		}
	}
	close(logs_fd);
	close(idx_fd);
	close(fifo_fd);
}
