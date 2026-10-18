#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "include/utils.h"


#define BROADCAST_ADDR INADDR_BROADCAST
#define DISCOVERY_PORT 9555
#define BASE_GAME_PORT 7000 
#define MAGIC_SECRET "DOCO"


int bind_game_port(int increment) {

    // run once at startup to bind 
    // and send on broadcast

    // returns sockfd of bound socket
    // exits on bind failure

    // init socket
    int sockfd;
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) ewerror("comm", "bind_game_port", "failed to initalize socket", 2);

    // set conn info
    struct sockaddr_in myaddr;
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(BASE_GAME_PORT + increment);
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(myaddr.sin_zero, '\0', sizeof myaddr.sin_zero);

    // recursively bind w increment and return sockfd
    int r = bind(sockfd, &myaddr, sizeof myaddr);
    if (r == 0) return sockfd;
    else if (errno == EADDRINUSE) return bind_game_port(increment+1);
    else ewerror("comm", "bind_game_port", "failed to bind to socket", 2);
}


int listen_for_peers(char** names, char** ips, char** ports, char** lastseens) {
    // listen for broadcasts
    // TODO

    
}


int hello_i_exist() {

    // send broadcasts
    // TODO

    // init socket
    int sockfd;
    sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == -1) ewerror("comm", "hello_i_exist", "failed to initalize socket", 2);

    // set broadcasting mode
    int broadcast = 1;
    int r = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast,sizeof broadcast);
    if (r == -1) ewerror("comm", "hello_i_exist", "failed to set broadcasting mode", 2);

    // init dest struct
    struct sockaddr_in broadcast_addr;
    memset(&broadcast_addr, 0, sizeof broadcast_addr);
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(DISCOVERY_PORT);
    broadcast_addr.sin_addr.s_addr = htonl(BROADCAST_ADDR);

    // make message


    return 0;
}