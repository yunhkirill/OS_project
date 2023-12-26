#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <string.h>

#define MAX_TYPE_LENGTH 50

int readWipingTime(int times[], char dishes[][256]) {
    FILE* ftimes = fopen("dried_dishes.txt", "r");
    if (ftimes == NULL) {
        perror("No wiping_time.txt file\n");
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

int wipeTime(int times[], char dishes[][256], char current_dish[256], int dishes_count) {
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
    int dish_count = readWipingTime(times, dishes);
    if (dish_count == -1) {
        return 1;
    }

    key_t key = ftok("token", 1);
    if (key == -1) {
        perror("ftok");
        return 2;
    }

    int semid = semget(key, 3, 0664);
    if (semid < 0) {
        perror("semget");
        return 3;
    }

    struct sembuf upper_restriction = {.sem_num = 1, .sem_op = +1, .sem_flg = 0};  // 1 index
    struct sembuf lower_restriction = {.sem_num = 2, .sem_op = -1, .sem_flg = 0};  // 2 index
    struct sembuf file_access_take = {.sem_num = 0, .sem_op = -1, .sem_flg = 0};    // 0 index
    struct sembuf file_access_turn_back = {.sem_num = 0, .sem_op = +1, .sem_flg = 0};

    int i = 1;
    while (1) {
        if (semop(semid, &lower_restriction, 1) < 0) {
            perror("semop lower_restriction\n");
            return 3;
        }

        if (semop(semid, &upper_restriction, 1) < 0) {
            perror("semop upper_restriction\n");
            return 3;
        }

        semop(semid, &file_access_take, 1);

        FILE* table_file = fopen("table.txt", "r");
        if (table_file == NULL) {
            perror("fopen");
            return 4;
        }

        char dish_to_wipe[256];
        for (int j = 0; j < i; ++j) {
            if (fscanf(table_file, "%s", dish_to_wipe) == -1) {
                perror("fscanf");
                fclose(table_file);
                return 4;
            }

            if (strcmp(dish_to_wipe, "end") == 0) {
                remove("table");
                semctl(semid, -1, IPC_RMID, NULL);
                return 0;
            }
        }

        fclose(table_file);

        semop(semid, &file_access_turn_back, 1);

        sleep(wipeTime(times, dishes, dish_to_wipe, dish_count));
        printf("WIPED %s\n", dish_to_wipe);
        ++i;
    }
    return 0;
}
