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

int main(int argc, char* argv[])
{

    bool costCutting = false;
    bool logOn = false;

    if (argc == 2) {
        if (sequal(argv[1], "--cost-cutting")) costCutting = true;
        else if (sequal(argv[1], "--log")) logOn = true;
        else ewerror(NULL, NULL, "invalid argument\n", 1);
    }

    if (argc == 3) {
        if (
            (sequal(argv[1], "--cost-cutting") || sequal(argv[1], "--log")) &&
            (sequal(argv[2], "--cost-cutting") || sequal(argv[2], "--log"))
        ) {
            costCutting = true;
            logOn = true;
        } else {
            ewerror(NULL, NULL, "invalid argument(s)\n", 1);
        }
    }

    printf("[DEBUG] CostCutting: %d\n", costCutting);
    printf("[DEBUG] LogOn: %d\n", logOn);
    return 0;
}
