#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>

#include "include/utils.h"

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


int pfd_add(struct pollfd** pfds, size_t* cap, int* n, int fd, short events) {

    // returns the index of the new entry

    if ((size_t) *n == *cap) {
        *cap = (*cap == 0) ? 8 : *cap * 2;
        *pfds = realloc(*pfds, *cap * sizeof **pfds);
        if (*pfds == NULL) ewerror("utils", "pfd_add", "out of memory\n", 1);
    }

    (*pfds)[*n] = (struct pollfd) { .fd = fd, .events = events };
    return (*n)++;
}
