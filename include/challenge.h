#ifndef CHALLENGE_H
#define CHALLENGE_H

#include <stddef.h>
#include <poll.h>

#include "comm.h"

enum mode {
    IDLE,
    CONNECTING, WAIT_WHO, WAIT_REPLY, // we challenged someone
    WAIT_ID, PROMPT,                  // someone challenged us
    IN_GAME,                          // handshake done
};

void challenge_init(int listenSock, char* myName, int myPort);
int challenge_build_pollfds(struct pollfd** pfds, size_t* cap, int used);
void challenge_handle_events(struct pollfd* pfds, int from, int count);
void challenge_check_timeout();
void challenge_handle_input(char* line, struct peer* peers, int peerCount);

#endif
