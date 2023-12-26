#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_TYPE_LENGTH 50

int readWashingTime(int times[], char dishes[][256]) {
    FILE* ftimes = fopen("washed_dishes.txt", "r");
    if (ftimes == NULL) {
        perror("No washing_time.txt file");
        return -1;
    }
    int time;
    char dish[256];
    int i = 0;
    while (fscanf(ftimes, "%s%d", dish, &time) != EOF) {
        sprintf(dishes[i], "%s", dish);
        times[i] = time;
        ++i;
    }
    fclose(ftimes);
    return i;
}

int washTime(int times[], char dishes[][256], char current_dish[256], int dishes_count) {
    int i = 0;
    while (strcmp(dishes[i], current_dish) != 0) {
        if (i >= dishes_count) {
            perror("No such dish");
            return -1;
        }
        ++i;
    }
    return times[i];
}

int main() {
    int N = atoi(getenv("TABLE_LIMIT"));
    int times[256];
    char dishes[256][256];
    int dish_count = readWashingTime(times, dishes);
    if (dish_count == -1) {
        return 1;
    }

    int dish_quantity;
    char dish_to_wash[256];
    FILE* wash_stream = fopen("dirty_dishes.txt", "r");
    if (wash_stream == NULL) {
        perror("No dish_pool.txt file");
        return 1;
    }

    if (creat("table.txt", 0666) == -1) {
        perror("Couldn't create table file");
        return 5;
    }

    key_t key = ftok("./../token", 1);
    if (key == -1) {
        perror("ftok");
        return 2;
    }

    int semid = semget(key, 3, IPC_CREAT | 0664);
    if (semid < 0) {
        perror("semget");
        return 3;
    }

    struct sembuf upper_restriction = {.sem_num = 1, .sem_op = -1, .sem_flg = 0};  // 1 index
    struct sembuf upper_restriction_fill = {.sem_num = 1, .sem_op = +N, .sem_flg = 0};
    struct sembuf lower_restriction = {.sem_num = 2, .sem_op = +1, .sem_flg = 0};  // 2 index
    struct sembuf file_access_take = {.sem_num = 0, .sem_op = -1, .sem_flg = 0};    // 0 index
    struct sembuf file_access_turn_back = {.sem_num = 0, .sem_op = +1, .sem_flg = 0};

    if (semop(semid, &file_access_turn_back, 1) < 0) {
        perror("semop file_access_turn_back\n");
        return 3;
    }

    if (semop(semid, &upper_restriction_fill, 1) < 0) {
        perror("semop upper_restriction_fill\n");
        return 3;
    }

    while (fscanf(wash_stream, "%s%d", dish_to_wash, &dish_quantity) != EOF) {
        while (dish_quantity != 0) {
            sleep(washTime(times, dishes, dish_to_wash, dish_count));

            if (semop(semid, &upper_restriction, 1) < 0) {
                perror("semop upper_restriction\n");
                return 3;
            }
            if (semop(semid, &lower_restriction, 1) < 0) {
                perror("semop lower_restriction\n");
                return 3;
            }

            semop(semid, &file_access_take, 1);

            FILE* table_file = fopen("table.txt", "a");
            if (table_file == NULL) {
                perror("fopen");
                return 4;
            }

            fprintf(table_file, "%s\n", dish_to_wash);
            fclose(table_file);

            semop(semid, &file_access_turn_back, 1);

            --dish_quantity;
            printf("WASHED %s\n", dish_to_wash);
        }
    }

    if (semop(semid, &lower_restriction, 1) < 0) {
        perror("semop lower_restriction\n");
        return 3;
    }

    semop(semid, &file_access_take, 1);

    FILE* table_file = fopen("table.txt", "a");
    if (table_file == NULL) {
        perror("fopen");
        return 4;
    }

    fprintf(table_file, "end\n");
    fclose(table_file);

    semop(semid, &file_access_turn_back, 1);

    return 0;
}