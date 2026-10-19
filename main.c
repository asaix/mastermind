#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <poll.h>
#include <unistd.h>

#include "include/utils.h"
#include "include/term.h"
#include "include/comm.h"
#include "include/challenge.h"
#include "include/master.h"
#include "include/breaker.h"

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

    // game screen shown, even after disconnect
    bool inGame = false;
    bool isMaster = false;
    char opponent[NAME_FIELD_SIZE];

    cls();
    draw_prompt();

    // rebuilt every pass since challenge connections come and go
    struct pollfd* pfds = NULL;
    size_t pfdsCap = 0;
    while (true) {

        int nfds = 0;
        pfd_add(&pfds, &pfdsCap, &nfds, discSock, POLLIN); // listen_for_peers drains it itself
        int stdinIdx = pfd_add(&pfds, &pfdsCap, &nfds, STDIN_FILENO, POLLIN);

        // the game socket while we're in a game and it's still open
        int gameIdx = -1;
        int sideFd = isMaster ? master_fd() : breaker_fd();
        if (sideFd != -1) {
            gameIdx = pfd_add(&pfds, &pfdsCap, &nfds, sideFd, POLLIN);
        }

        int challengeFrom = nfds;
        challenge_add_pollfds(&pfds, &pfdsCap, &nfds);

        // wakes on a broadcast, enter key in stdin, challenge or game traffic, or after 1 s
        if (poll(pfds, nfds, 1000) == -1) ewerror("main", NULL, "poll failed\n", 2);

        // before stdin, so a connection opened by a typed command never sees this pass's revents
        challenge_handle_events(pfds, challengeFrom, nfds);
        if (gameIdx != -1 && pfds[gameIdx].revents != 0) {
            if (isMaster) master_socket();
            else breaker_socket();
        }

        if (pfds[stdinIdx].revents & (POLLIN | POLLHUP)) {
            char line[64];
            if (fgets(line, sizeof line, stdin) == NULL) {
                // EOF (ctrl+d)
                // ignore and reset dirty
                clearerr(stdin);
            } else {
                if (inGame && isMaster) master_input(line);
                else if (inGame) breaker_input(line);
                else challenge_handle_input(line, peers, peerCount);
                draw_prompt();
            }
        }

        challenge_check_timeout();

        // hs ok: load game screen
        int newGame = challenge_game_started(opponent, &isMaster);
        if (newGame != -1) {
            inGame = true;
            cls();
            draw_prompt();
            if (isMaster) master_start(newGame, opponent);
            else breaker_start(newGame, opponent);
        }

        if (time(NULL) - lastHello >= BROADCAST_FREQ) {
            hello_i_exist(discSock, name, gamePort);
            lastHello = time(NULL);
        }

        if (listen_for_peers(discSock, peers, &peerCount) == -1) {
            ewerror("main", NULL, "failed to listen for peers\n", 2);
        }

        if (!inGame) draw_peers(peers, peerCount);
    }
}
