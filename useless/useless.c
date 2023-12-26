#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGUMENTS 16

void execute_command(char *command) {
    char *args[MAX_ARGUMENTS];
    char *token;
    int i = 0;

    token = strtok(command, " ");
    while (token != NULL && i < MAX_ARGUMENTS - 1) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    execvp(args[0], args);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Wrong number of input arguments.\n");
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("Error opening input file");
        exit(errno);
    }

    char command[MAX_COMMAND_LENGTH];
    int sleep_time;
    int scan_check;
    //while (!feof(file)) {
        while ((scan_check = fscanf(file, "%d %[^\n]", &sleep_time, command)) == 2) {
            if (sleep_time < 0) {
                printf("Wrong time format.\n");
                continue;
		    }
            pid_t pid = fork();
            if (pid == -1) {
                perror("Fork error");
                exit(errno);
            } else if (pid == 0) {
                sleep(sleep_time);
                execute_command(command);
                printf("Error executing command\n");
                _exit(EXIT_FAILURE);
            }
            wait(NULL);
        }
        if (ferror(file)) {
            perror("Input/output error");
        } else if (scan_check != EOF) {
			printf("Wrong input format\n");
            fscanf(file, "%*[^\n]]");
		}
    //}
    wait(NULL);
    fclose(file);
    return 0;
}
