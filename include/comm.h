#ifndef COMM_H
#define COMM_H

#include <time.h>
#include <netinet/in.h>

#include "term.h"

#define NAME_FIELD_SIZE (MAX_NAME_SIZE + 1) // so there's always at least 1 pad byte


#define MAX_PEERS 10
#define BROADCAST_FREQ 2 // seconds between discovery broadcasts

// challenge handshake format
//   B -> A  WHO ARE YOU
//   A -> B  <name>\r\n<port>
//   B -> A  OK | NAH | BUSY
#define MSG_END "\r\n\r\n"
#define MSG_END_LEN 4
#define MSG_SEP "\r\n" // between fields inside a message
#define MSG_WHO "WHO ARE YOU"
#define MSG_OK "OK"
#define MSG_NAH "NAH"
#define MSG_BUSY "BUSY"

// game messages, also end with MSG_END
//   mastermind -> codebreaker  READY                      (master sequence set, guesses allowed from now)
//   codebreaker -> mastermind  GUESS:<guess>
//   mastermind -> codebreaker  FEEDBACK:<feedback>
//   mastermind -> codebreaker  REVEAL:<master sequence>
#define MSG_READY "READY"
#define MSG_GUESS "GUESS:"
#define MSG_FEEDBACK "FEEDBACK:"
#define MSG_REVEAL "REVEAL:"

#define CHALLENGE_TIMEOUT 10
#define MSG_BUF_SIZE 64

struct peer {
    char name[NAME_FIELD_SIZE];
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

// tcp challenge
int start_connect(char* ip, int port);
int finish_connect(int sockfd);
int send_msg(int sockfd, char* body);
int recv_into(int sockfd, char* buf, size_t* len);
int xmsg(int sockfd, char* buf, size_t* len, char* out, size_t outSize);
void close_conn(int sockfd);
#endif
