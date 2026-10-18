#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <poll.h>

#include "include/utils.h"
#include "include/term.h"
#include "include/comm.h"

// err 1: generic error
// err 2: network error


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

    int discSock = init_discovery_sock();
    int gameSock = bind_game_port(0);
    int gamePort = get_bound_port(gameSock);

    struct peer peers[MAX_PEERS];
    int peerCount = 0;
    time_t lastHello = 0; // 0 so the first broadcast goes out immediately

    cls();
    draw_prompt();

    struct pollfd pfd = { .fd = discSock, .events = POLLIN };
    while (true) {

        // wakes on a broadcast or after 1 s, whichever comes first
        if (poll(&pfd, 1, 1000) == -1) ewerror("main", NULL, "poll failed\n", 2);

        if (time(NULL) - lastHello >= BROADCAST_FREQ) {
            hello_i_exist(discSock, name, gamePort);
            lastHello = time(NULL);
        }

        if (listen_for_peers(discSock, peers, &peerCount) == -1) {
            ewerror("main", NULL, "failed to listen for peers\n", 2);
        }

        draw_peers(peers, peerCount);
    }
}
