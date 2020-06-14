

int isExecute(char *str);
void sigint_handler(int signum);
void timeout_handler_pipe(int signum);
void timeout_handler(int signum);
void timeout_handler_pipe_aux(int signum);
void sigchld_handler(int signum);
void sigtermallchildren_handler(int signum);
void sigtermallchildren_handler2(int signum);
int mysystem(char *command, int logs_fd, int idx_fd);
int exec_command(char* command);
int parser(char **command, char *argv);
void mypiping(char *argv, int logs_fd, int idx_fd);