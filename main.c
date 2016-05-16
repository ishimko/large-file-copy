#include <libgen.h>
#include <stdlib.h>
#include <fcntl.h>
#include <memory.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include "error_printer.h"

#define ARGS_COUNT 4
#define BUFFER_SIZE 4096
char *MODULE_NAME;

int copy_file(const char *src_path, const char *dest_path, int processes_count) {
    char buffer[BUFFER_SIZE];
    int src, dest;

    if ((src = open(src_path, O_RDONLY)) == -1) {
        print_error(MODULE_NAME, strerror(errno), src_path);
        return -1;
    }

    if ((dest = open(dest_path, O_WRONLY | O_CREAT | O_EXCL)) == 1) {
        print_error(MODULE_NAME, strerror(errno), dest_path);
        if (close(src) == -1) {
            print_error(MODULE_NAME, strerror(errno), src_path);
        }
        return -1;
    }
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

    char dest_path[PATH_MAX];
    char src_path[PATH_MAX];

    realpath(argv[1], src_path);
    realpath(argv[2], dest_path);

    return copy_file(src_path, dest_path, processes_number);
}