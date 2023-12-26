#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

int zip(char *path) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("Fork error");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        execlp("gzip", "gzip", "-Nr", path, NULL);
        perror("Error zip");
        _exit(EXIT_FAILURE);
    }
    wait(NULL);
    return 0;
}

int unzip(char *path) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }
    pid_t pid = fork();
    if (pid == -1) {
        perror("Fork error");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        execlp("gzip", "gzip", "-dNr", path, NULL);
        perror("Error unzip");
        _exit(EXIT_FAILURE);
    }
    wait(NULL);
    return 0;
}

int backup(char *source_path, char *destination_path) {
    pid_t pid;
    DIR *source_dir = opendir(source_path);
    if (source_dir == NULL) {
        perror("Error opening source directory");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    char *args[4096];
    char buffer[4096];
    int arg_count = 2, k;

    args[0] = "cp";
    args[1] = "-rp";

    DIR *destination_dir = opendir(destination_path);
    if (destination_dir == NULL) {
        perror("Error creating or opening destination directory");
        exit(EXIT_FAILURE);
    }

    entry = readdir(source_dir);
    if (entry == NULL) {
        printf("Source directory is empty.\n");
        closedir(source_dir);
        closedir(destination_dir);
        exit(EXIT_FAILURE);
    }

    rewinddir(source_dir);

    while ((entry = readdir(source_dir))) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;
        // entry->d_ino - Уникальный идентификатор индексного узла в рамках файловой системы.
        //(.) - ссылка на текущую директорию; (..) - ссылка на родительскую директорию
        snprintf(buffer, sizeof(buffer), "%s/%s", source_path, entry->d_name);
        args[arg_count] = strdup(buffer);

        if (!args[arg_count]) {
            perror("Memory allocation error");
            exit(EXIT_FAILURE);
        }

        arg_count++;
    }

    args[arg_count] = destination_path;
    closedir(source_dir);
    closedir(destination_dir);

    pid = fork();
    if (pid == -1) {
        perror("Fork error");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        execvp(args[0], args);
        perror("execvp");
        _exit(EXIT_FAILURE);
    }

    wait(NULL);

    for (k = 2; k < arg_count; k++) {
        free(args[k]);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Wrong number of arguments.\n");
        exit(EXIT_FAILURE);
    }

    unzip(argv[2]);
    backup(argv[1], argv[2]);
    zip(argv[2]);

    return 0;
}