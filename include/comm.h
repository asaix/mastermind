#ifndef COMM_H
#define COMM_H

#include <time.h>
#include <netinet/in.h>

#include "term.h"

#define MAX_PEERS 10
#define BROADCAST_FREQ 2 // seconds between discovery broadcasts

struct peer {
    char name[MAX_NAME_SIZE + 1];
    char ip[INET_ADDRSTRLEN];
    int port;
    time_t lastSeen;
};

int bind_game_port(int increment);
int get_bound_port(int sockfd);

// discovery
int init_discovery_sock();
int hello_i_exist(int sockfd, char* name, int port);
int listen_for_peers(int sockfd, struct peer* peers, int* peerCount);

#endif
