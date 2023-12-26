#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_COMMAND_LENGTH 1024

int max_processes;
int running_processes = 0;

void sigchld_handler() {
    int status;
    pid_t pid;
    //что возвращает waitpid
    //Если какой-то дочерний процесс завершается, waitpid возвращает его идентификатор (PID), и цикл продолжает выполняться.
    //Используется для неблокирующего ожидания завершения любого дочернего процесса.
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            printf("Process %d has exited.\n", pid);
        } else if (WIFSIGNALED(status)) {
            printf("Process %d has been killed.\n", pid);
        }
        running_processes--;
    }
}

void runsim() {
    signal(SIGCHLD, sigchld_handler);
    //какие бывают сигналы
    //рекация по умолчанию

//          При завершении работы лидера сеанса все
// процессы из текущей группы сеанса получают
// сигнал SIGHUP 
//          SIGHUP по умолчанию приводит к завершению
// процесса(работу продолжат только фоновые процессы)

//         SIGCHLD (завершение дочернего процесса) 
//         Когда дочерний процесс завершается, система автоматически игнорирует
// сигнал SIGCHLD без генерации дополнительных действий.

//         SIGINT (прерывание с клавиатуры, обычно вызывается по нажатию Ctrl+C)
//          реакция по умолчанию заключается в том, что процесс завершается.
    char command[MAX_COMMAND_LENGTH];

    while (1) {
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }

        command[strcspn(command, "\n")] = '\0';

        if (running_processes >= max_processes) {
            printf("Error: Too many commands running, please wait until some exit.\n");
            continue;
        }

        pid_t pid = fork();

        if (pid == -1) {
            perror("Fork error");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            execlp(command, command, NULL);
            perror("Command execution error");
            _exit(EXIT_FAILURE);
        } else {
            running_processes++;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Wrong number of input arguments.\n");
        exit(EXIT_FAILURE);
    }

    max_processes = atoi(argv[1]);

    if (max_processes <= 0) {
        printf("Invalid value for max_processes.\n");
        exit(EXIT_FAILURE);
    }

    runsim();

    return 0;
}
