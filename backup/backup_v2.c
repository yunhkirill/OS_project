#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

#define PATH_MAX 1024

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

void make_path(char *path_from, const char *path_to) {
    if (snprintf(path_from, PATH_MAX, "%s/%s", path_from, path_to) >= PATH_MAX) {
        fprintf(stderr, "Error: Path exceeds maximum length\n");
        exit(EXIT_FAILURE);
    }
}

int is_file_in_directory(const char *directory_path, const char *filename) {
    DIR *dir = opendir(directory_path);

    if (dir == NULL) {
        perror("Error opening directory");
        return -1;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, filename) == 0) {
            closedir(dir);
            return 1;
        }
    }

    closedir(dir);
    return 0;  
}

void backup(const char *source_path, const char *destination_path) {
    pid_t pid;
    DIR *source_dir = opendir(source_path);
    if (source_dir == NULL) {
        perror("Error opening source directory");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;

    DIR *destination_dir = opendir(destination_path);
    if (destination_dir == NULL) {
        perror("Error creating or opening destination directory");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(source_dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_source_path[PATH_MAX];
        char full_destination_path[PATH_MAX];

        snprintf(full_source_path, PATH_MAX, "%s/%s", source_path, entry->d_name);
        snprintf(full_destination_path, PATH_MAX, "%s/%s", destination_path, entry->d_name);

        struct stat source_statbuf;
        if (lstat(full_source_path, &source_statbuf) != 0)
            perror("Error source statbuf");

        char full_destination_path_gz[PATH_MAX];
        strcpy(full_destination_path_gz, full_destination_path);
        strcpy(full_destination_path_gz, ".gz");
        if (is_file_in_directory(destination_path, full_destination_path_gz) == 1) {
            struct stat destination_statbuf;
			if (lstat(full_destination_path_gz, &destination_statbuf) != 0)
				perror("Error destination statbuf");
			if (source_statbuf.st_mtime <= destination_statbuf.st_mtime)
                continue;
        }
    

        if (S_ISDIR(source_statbuf.st_mode)) {
            mkdir(full_destination_path, source_statbuf.st_mode);
            backup(full_source_path, full_destination_path);
        } else if (S_ISREG(source_statbuf.st_mode)) {
            pid = fork();
            if (pid == -1) {
                perror("Fork error");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                execlp("cp", "cp", "-rp", full_source_path, full_destination_path, NULL);
                perror("Error during copy");
                _exit(EXIT_FAILURE);
            }
        } else if (S_ISLNK(source_statbuf.st_mode))
            continue;
        wait(NULL);
    }

    closedir(source_dir);
    closedir(destination_dir);
}
        


int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Wrong number of arguments.\n");
        exit(EXIT_FAILURE);
    }

    backup(argv[1], argv[2]);
    zip(argv[2]);

    return 0;
}
