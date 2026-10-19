#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <poll.h>
#include <unistd.h>

#include "include/utils.h"
#include "include/term.h"
#include "include/comm.h"
#include "include/challenge.h"

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

    challenge_init(gameSock, name, gamePort);

    cls();
    draw_prompt();

    // rebuilt every pass since challenge connections come and go
    struct pollfd* pfds = NULL;
    size_t pfdsCap = 0;
    while (true) {

        int nfds = challenge_build_pollfds(&pfds, &pfdsCap, 2);
        pfds[0] = (struct pollfd) { .fd = discSock, .events = POLLIN };
        pfds[1] = (struct pollfd) { .fd = STDIN_FILENO, .events = POLLIN };

        // wakes on a broadcast, enter key in stdin, challenge traffic, or after 1 s
        if (poll(pfds, nfds, 1000) == -1) ewerror("main", NULL, "poll failed\n", 2);

        // before stdin, so a connection opened by a typed command never sees this pass's revents
        challenge_handle_events(pfds, 2, nfds);

        if (pfds[1].revents & (POLLIN | POLLHUP)) {
            char line[64];
            if (fgets(line, sizeof line, stdin) == NULL) {
                // EOF (ctrl+d)
                // ignore and reset dirty
                clearerr(stdin);
            } else {
                challenge_handle_input(line, peers, peerCount);
                draw_prompt();
            }
        }

        challenge_check_timeout();

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
