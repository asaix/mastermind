#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/term.h"

char* name_entry() {
    int MAX_NAME_SIZE = 15;
    char* name = malloc(MAX_NAME_SIZE * 2 * sizeof(char));

    fputs("Enter your name: ", stdout);
    fgets(name, MAX_NAME_SIZE + 1, stdin); // +1 for \0

    // drop trailing newline if present
    size_t newlineIndex = strcspn(name, "\n");
    name[newlineIndex] = '\0';

    return name;

}  