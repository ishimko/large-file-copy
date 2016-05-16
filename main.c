#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>

#define ARGS_COUNT 4

char *MODULE_NAME;

void print_error(const char *module_name, const char *error_msg, const char *additional_info) {
    fprintf(stderr, "%s: %s", module_name, error_msg);
    if (additional_info){
        fprintf(stderr, ": %s", additional_info);
    }
    fputs("\n", stderr);
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

    char *src_path = argv[1];
    char *dest_path = argv[2];
}