#include <stdio.h>
#include <stdlib.h>

#include "include/utils.h"
#include "include/term.h"

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

    char* name = name_entry();
    printf("[DEBUG] Name: %s\n", name);
    return 0;
}
