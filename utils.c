#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool sequal(char* s1, char*s2) {
    if (strcmp(s1, s2) == 0) return true;
    return false;
}

void ewerror(char* module, char* fn, char* msg, int exitCode) {
    if (module == NULL) {
        fprintf(stderr, "mastermind: %s", msg);
        exit(exitCode);
    }
    if (fn == NULL) {
        fprintf(stderr, "mastermind:%s: %s", module, msg);
        exit(exitCode);
    }

    fprintf(stderr, "mastermind:%s:%s: %s", module, fn, msg);
    exit(exitCode);
}