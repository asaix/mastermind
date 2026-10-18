#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "include/utils.h"
#include "include/term.h"
#include "include/comm.h"


#define BROADCAST_ADDR INADDR_BROADCAST
#define DISCOVERY_PORT 9555
#define BASE_GAME_PORT 7000
#define MAGIC_SECRET "LIES"

// discovery packet: [magic][session id, network order][name, pkcs#7 padded][port as ascii digits]
#define MAGIC_SIZE 4
#define ID_SIZE 2
#define NAME_FIELD_SIZE (MAX_NAME_SIZE + 1) // so there's always at least 1 pad byte
#define PORT_DIGITS_MAX 5
#define DISCOVERY_MSG_MAX (MAGIC_SIZE + ID_SIZE + NAME_FIELD_SIZE + PORT_DIGITS_MAX)
#define PORT_OFFSET (MAGIC_SIZE + ID_SIZE + NAME_FIELD_SIZE)

#define PEER_TIMEOUT 3 * BROADCAST_FREQ // seconds without a broadcast before a peer is dropped


// random per run, used to spot our own broadcasts
static uint16_t sessionId;


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
    int r = bind(sockfd, (struct sockaddr*) &myaddr, sizeof myaddr);
    if (r == 0) return sockfd;
    else if (errno == EADDRINUSE) return bind_game_port(increment+1);
    else ewerror("comm", "bind_game_port", "failed to bind to socket", 2);
}


int listen_for_peers(int sockfd, struct peer* peers, int* peerCount) {

    // read every queued broadcast on the discovery socket without blocking
    // adds new peers, refreshes known ones (same ip + port)
    // then drops peers not seen for PEER_TIMEOUT seconds

    // returns 1 if the peer list changed, 0 if not
    // returns -1 on poll / recv failure

    int changed = 0;
    struct pollfd pfd = { .fd = sockfd, .events = POLLIN };

    // 0 timeout so poll returns right away, loop until the queue is empty
    int r;
    while ((r = poll(&pfd, 1, 0)) > 0 && (pfd.revents & POLLIN)) {

        unsigned char buf[DISCOVERY_MSG_MAX];
        struct sockaddr_in src;
        socklen_t srcLen = sizeof src;
        ssize_t n = recvfrom(sockfd, buf, sizeof buf, 0, (struct sockaddr*) &src, &srcLen);
        if (n == -1) return -1;

        // not from this program
        if (memcmp(buf, MAGIC_SECRET, MAGIC_SIZE) != 0) continue;

        // our own broadcast
        uint16_t netId;
        memcpy(&netId, buf + MAGIC_SIZE, ID_SIZE);
        if (ntohs(netId) == sessionId) continue;

        // get name
        // pkcs7 so last byte is pad count
        // note: not doing the full pkcs7 check on all padding bytes
        unsigned char* nameField = buf + MAGIC_SIZE + ID_SIZE;
        int nameLen = NAME_FIELD_SIZE - nameField[NAME_FIELD_SIZE - 1];

        // get port (not null term'd)
        char portStr[PORT_DIGITS_MAX + 1];
        int portLen = n - PORT_OFFSET;
        memcpy(portStr, buf + PORT_OFFSET, portLen);
        portStr[portLen] = '\0';
        int port = atoi(portStr);

        // get IP from pkt
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &src.sin_addr, ip, sizeof ip);

        // find a known peer, or append if there's room
        int i;
        for (i = 0; i < *peerCount; i++) {
            if (sequal(peers[i].ip, ip) && peers[i].port == port) break;
        }
        if (i == *peerCount) {
            // new peer

            // no space, skip
            if (*peerCount == MAX_PEERS) continue;

            // add to list
            strcpy(peers[i].ip, ip);
            peers[i].port = port;
            (*peerCount)++;
        }

        memcpy(peers[i].name, nameField, nameLen);
        peers[i].name[nameLen] = '\0';
        peers[i].lastSeen = time(NULL);
        changed = 1;
    }
    if (r == -1) return -1;

    // drop stale peers, shifting the rest down so the order stays the same
    time_t now = time(NULL);
    for (int i = 0; i < *peerCount; ) {
        if (now - peers[i].lastSeen > PEER_TIMEOUT) {
            memmove(&peers[i], &peers[i + 1], (*peerCount - i - 1) * sizeof *peers);
            (*peerCount)--;
            changed = 1;
        } else {
            i++;
        }
    }

    return changed;
}


int get_bound_port(int sockfd) {

    // returns the local port sockfd is bound to
    // returns -1 on failure

    struct sockaddr_in addr;
    socklen_t len = sizeof addr;
    int r = getsockname(sockfd, (struct sockaddr*) &addr, &len);
    if (r == -1) return -1;
    return ntohs(addr.sin_port);
}


int init_discovery_sock() {

    // run once at startup
    // the same socket is used to send and listen for broadcasts

    // returns sockfd of socket bound to the discovery port
    // exits on failure

    // pick session id, pid in the seed so instances started in the same second differ
    srand(time(NULL) ^ getpid());
    sessionId = rand() & 0xFFFF; // keep only lower 16 bits

    // init socket
    int sockfd;
    sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == -1) ewerror("comm", "init_discovery_sock", "failed to initalize socket", 2);

    // set broadcasting mode
    int broadcast = 1;
    int r = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast);
    if (r == -1) ewerror("comm", "init_discovery_sock", "failed to set broadcasting mode", 2);

    // let other instances on the same host bind the discovery port too
    int reuse = 1;
    r = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);
    if (r == -1) ewerror("comm", "init_discovery_sock", "failed to set address reuse", 2);

    // set conn info
    struct sockaddr_in myaddr;
    memset(&myaddr, 0, sizeof myaddr);
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(DISCOVERY_PORT);
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    r = bind(sockfd, (struct sockaddr*) &myaddr, sizeof myaddr);
    if (r == -1) ewerror("comm", "init_discovery_sock", "failed to bind to socket", 2);

    return sockfd;
}


int hello_i_exist(int sockfd, char* name, int port) {

    // send one discovery broadcast on the bound discovery socket

    // returns 0 on success
    // returns -1 on bad name / port or send failure

    size_t nameLen = strlen(name);
    if (nameLen > MAX_NAME_SIZE) return -1;
    if (port < 1 || port > 65535) return -1;

    char msg[DISCOVERY_MSG_MAX + 1]; // +1 for the \0 snprintf writes, not sent
    size_t msgLen = 0;

    // cpy magic w/o null terminator
    memcpy(msg, MAGIC_SECRET, MAGIC_SIZE);
    msgLen += MAGIC_SIZE;

    // session id
    uint16_t netId = htons(sessionId);
    memcpy(msg + msgLen, &netId, ID_SIZE);
    msgLen += ID_SIZE;

    // PKCS#7 padding for name
    memcpy(msg + msgLen, name, nameLen);
    unsigned char pad = NAME_FIELD_SIZE - nameLen;
    memset(msg + msgLen + nameLen, pad, pad);
    msgLen += NAME_FIELD_SIZE;

    // port, takes up the rest of the packet
    int digits = snprintf(msg + msgLen, sizeof msg - msgLen, "%d", port);
    msgLen += digits; // don't include \0

    // init dest struct
    struct sockaddr_in broadcast_addr;
    memset(&broadcast_addr, 0, sizeof broadcast_addr);
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(DISCOVERY_PORT);
    broadcast_addr.sin_addr.s_addr = htonl(BROADCAST_ADDR);

    // udp sends msgLen in the header so we don't have to attach it
    ssize_t sent = sendto(sockfd, msg, msgLen, 0, (struct sockaddr*) &broadcast_addr, sizeof broadcast_addr);
    if (sent != (ssize_t) msgLen) return -1;

    return 0;
}