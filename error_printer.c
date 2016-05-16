#include "error_printer.h"
#include <stdio.h>

void print_error(const char *module_name, const char *error_msg, const char *additional_info) {
    fprintf(stderr, "%s: %s", module_name, error_msg);
    if (additional_info){
        fprintf(stderr, ": %s", additional_info);
    }
    fputs("\n", stderr);
}
