#include <libgen.h>
#include <stdlib.h>
#include <fcntl.h>
#include <memory.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "error_printer.h"

#define ARGS_COUNT 4
#define BUFFER_SIZE 4096

char *MODULE_NAME;
char SRC_PATH[PATH_MAX];
char DEST_PATH[PATH_MAX];

struct sembuf LOCK = {.sem_op = -1};
struct sembuf UNLOCK = {.sem_op = 1};

static int safe_write(int dest, const char *buffer, off_t start_offset, size_t size, int sem_id) {
    if (semop(sem_id, &LOCK, 1) == -1) {
        print_error(MODULE_NAME, strerror(errno), "locking semaphore");
        return -1;
    }

    if (lseek(dest, start_offset, SEEK_SET) == -1) {
        print_error(MODULE_NAME, strerror(errno), DEST_PATH);
        return -1;
    }

    if (write(dest, buffer, size) != size) {
        print_error(MODULE_NAME, strerror(errno), DEST_PATH);
        return -1;
    }

    if (semop(sem_id, &UNLOCK, 1) == -1) {
        print_error(MODULE_NAME, strerror(errno), "unlocking semaphore");
        return -1;
    }

    return 0;
}

static void copy_part(const char *src_path, int dest, int sem_id, off_t start_offset, ssize_t part_size) {
    char buffer[BUFFER_SIZE];
    int error = 0;
    ssize_t bytes_count = 0;
    int src;

    if ((src = open(src_path, O_RDONLY)) == -1) {
        print_error(MODULE_NAME, strerror(errno), src_path);
        return;
    }

    if (lseek(src, start_offset, SEEK_SET) == -1) {
        print_error(MODULE_NAME, strerror(errno), src_path);
        error = -1;
    }

    if (!error) {
        ssize_t full_buffers_count = part_size / BUFFER_SIZE;
        for (ssize_t i = 0; (i < full_buffers_count) && (bytes_count != -1) && (!error); i++) {
            bytes_count = read(src, buffer, BUFFER_SIZE);
            if (bytes_count != -1) {
                error = safe_write(dest, buffer, start_offset, BUFFER_SIZE, sem_id);
            } else {
                print_error(MODULE_NAME, strerror(errno), src_path);
                error = -1;
            }
            start_offset += BUFFER_SIZE;
        }

        if (!error) {
            size_t bytes_remain = (size_t) part_size % BUFFER_SIZE;
            if (read(src, buffer, bytes_remain) == -1) {
                print_error(MODULE_NAME, strerror(errno), src_path);
                error = -1;
            }

            if (!error) {
                if (write(dest, buffer, bytes_remain) == -1) {
                    print_error(MODULE_NAME, strerror(errno), DEST_PATH);
                }
            }
        }

    }

    if (close(src) == -1) {
        print_error(MODULE_NAME, strerror(errno), SRC_PATH);
    }
}

static ssize_t get_file_size(const char *path) {
    FILE *f = fopen(path, "r");
    ssize_t size;

    if (!f) {
        print_error(MODULE_NAME, strerror(errno), path);
        return -1;
    }

    if (fseek(f, 0, SEEK_END) == -1) {
        print_error(MODULE_NAME, strerror(errno), path);
        return -1;
    }

    if ((size = ftell(f)) == -1) {
        print_error(MODULE_NAME, strerror(errno), path);
        return -1;
    }

    if (fclose(f) == -1) {
        print_error(MODULE_NAME, strerror(errno), path);
    }

    return size;
}

static int copy_file(const char *src_path, const char *dest_path, int processes_count) {
    int dest;
    int sem_id;
    ssize_t full_size;
    struct stat file_info;

    if ((sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | IPC_EXCL | 0666)) == -1) {
        print_error(MODULE_NAME, strerror(errno), "semaphore creating");
        return -1;
    }

    if (semop(sem_id, &UNLOCK, 1) == -1) {
        print_error(MODULE_NAME, strerror(errno), "semaphore initializing");
        return -1;
    }

    if (stat(src_path, &file_info) == -1){
        print_error(MODULE_NAME, strerror(errno), src_path);
        return -1;
    }

    if ((dest = open(dest_path, O_WRONLY | O_CREAT | O_EXCL, file_info.st_mode)) == -1) {
        print_error(MODULE_NAME, strerror(errno), dest_path);
        return -1;
    }

    if ((full_size = get_file_size(src_path)) == -1) {
        print_error(MODULE_NAME, strerror(errno), src_path);
        return -1;
    }

    ssize_t part_size = full_size / processes_count;

    off_t start_offset = 0;

    pid_t child;
    for (int i = 1; i <= processes_count; i++) {
        child = fork();
        switch (child) {
            case 0:
                copy_part(src_path, dest, sem_id, start_offset,
                          (i == processes_count) ? (full_size - start_offset) : part_size);
                return 0;
            case -1:
                print_error(MODULE_NAME, strerror(errno), NULL);
                break;
            default:
                start_offset += part_size;
                break;
        }
    }

    while (wait(NULL) > 0);

    return 0;
}

int main(int argc, char *argv[]) {
    MODULE_NAME = basename(argv[0]);

    if (argc != ARGS_COUNT) {
        print_error(MODULE_NAME, "Wrong number of parameters", NULL);
        return 1;
    }

    int processes_number;
    if ((processes_number = atoi(argv[3])) < 1) {
        print_error(MODULE_NAME, "Number of processes must be positive", NULL);
        return 1;
    }

    realpath(argv[1], SRC_PATH);
    realpath(argv[2], DEST_PATH);

    return copy_file(SRC_PATH, DEST_PATH, processes_number);
}