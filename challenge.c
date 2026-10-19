#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>

#include "include/utils.h"
#include "include/term.h"
#include "include/comm.h"
#include "include/challenge.h"

#define STATUS_SIZE 96

struct conn {
    int fd;
    char buf[MSG_BUF_SIZE];
    size_t len;
};

// main imp handshake 
static enum mode mode = IDLE;
static struct conn active;
static time_t started;
static char peerName[MAX_NAME_SIZE + 1];
static int peerPort;

// connections that come in while we're busy, they get WHO ARE YOU then BUSY
static struct conn* extras = NULL;
static int extraCount = 0;

static int listenSock;
static char* myName;
static int myPort;


void challenge_init(int sockfd, char* name, int port) {

    // sockfd is the listening game socket from bind_game_port

    listenSock = sockfd;
    myName = name;
    myPort = port;
}


static void back_to_idle(char* msg) {

    // drop the active connection and stop being busy

    close(active.fd);
    active.fd = -1;
    mode = IDLE;
    draw_status(msg);
}


static void handle_accept() {

    int fd = accept(listenSock, NULL, NULL);
    if (fd == -1) return;

    if (mode == IDLE) {
        // this one becomes the active handshake, busy from here on
        active.fd = fd;
        active.len = 0;
        active.buf[0] = '\0';
        mode = WAIT_ID;
        started = time(NULL);
        if (send_msg(fd, MSG_WHO) == -1) back_to_idle("challenger disconnected");
        return;
    }

    if (send_msg(fd, MSG_WHO) == -1) {
        close(fd);
        return;
    }

    extras = realloc(extras, (extraCount + 1) * sizeof *extras);
    if (extras == NULL) ewerror("challenge", "handle_accept", "out of memory\n", 1);
    extras[extraCount].fd = fd;
    extras[extraCount].len = 0;
    extras[extraCount].buf[0] = '\0';
    extraCount++;
}


static void handle_id(char* msg) {

    // msg is <name>\r\n<port>

    char* sep = strstr(msg, MSG_SEP);
    if (sep == NULL) {
        back_to_idle("challenger disconnected");
        return;
    }
    *sep = '\0';
    snprintf(peerName, sizeof peerName, "%s", msg);
    peerPort = atoi(sep + strlen(MSG_SEP));

    mode = PROMPT;
    char status[STATUS_SIZE];
    snprintf(status, sizeof status, "%s wants to play. accept? (yes/no)", peerName);
    draw_status(status);
}


static void handle_active() {

    int r = recv_into(active.fd, active.buf, &active.len);
    if (r <= 0) {
        back_to_idle("challenger disconnected");
        return;
    }

    char msg[MSG_BUF_SIZE];
    while (mode != IDLE && xmsg(active.buf, &active.len, msg, sizeof msg)) {
        if (mode == WAIT_ID) handle_id(msg);
        // anything else the challenger sends before we answer is ignored
    }
}


static void handle_extra(int i) {

    // close once they've answered WHO ARE YOU (with BUSY) or gone away

    struct conn* c = &extras[i];
    char msg[MSG_BUF_SIZE];

    int r = recv_into(c->fd, c->buf, &c->len);
    if (r == 1 && !xmsg(c->buf, &c->len, msg, sizeof msg)) return;
    if (r == 1) send_msg(c->fd, MSG_BUSY);

    close(c->fd);
    extras[i] = extras[extraCount - 1];
    extraCount--;
}


int challenge_build_pollfds(struct pollfd** pfds, size_t* cap, int used) {

    // append the listening socket, active connection and extras after the first used entries
    // grows *pfds as needed

    // returns the total number of entries

    size_t need = used + 2 + extraCount;
    if (need > *cap) {
        *pfds = realloc(*pfds, need * sizeof **pfds);
        if (*pfds == NULL) ewerror("challenge", "challenge_build_pollfds", "out of memory\n", 1);
        *cap = need;
    }

    int n = used;
    (*pfds)[n++] = (struct pollfd) { .fd = listenSock, .events = POLLIN };
    if (mode != IDLE) {
        short events = (mode == CONNECTING) ? POLLOUT : POLLIN;
        (*pfds)[n++] = (struct pollfd) { .fd = active.fd, .events = events };
    }
    for (int i = 0; i < extraCount; i++) {
        (*pfds)[n++] = (struct pollfd) { .fd = extras[i].fd, .events = POLLIN };
    }

    return n;
}


void challenge_handle_events(struct pollfd* pfds, int from, int count) {

    // handle whatever poll reported on the entries challenge_build_pollfds added
    // the listening socket comes first, so new fds are made before any are closed this pass

    for (int i = from; i < count; i++) {
        if (pfds[i].revents == 0) continue;

        int fd = pfds[i].fd;
        if (fd == listenSock) {
            handle_accept();
        } else if (mode != IDLE && fd == active.fd) {
            handle_active();
        } else {
            for (int j = 0; j < extraCount; j++) {
                if (extras[j].fd == fd) {
                    handle_extra(j);
                    break;
                }
            }
        }
    }
}


void challenge_check_timeout() {
    if (mode != IDLE && time(NULL) - started >= CHALLENGE_TIMEOUT) back_to_idle("challenge timed out");
}


void challenge_handle_input(char* line, struct peer* peers, int peerCount) {

    // a line typed at the prompt

    line[strcspn(line, "\n")] = '\0';
    char status[STATUS_SIZE];

    if (mode == PROMPT) {
        if (sequal(line, "yes")) {
            send_msg(active.fd, MSG_OK);
            snprintf(status, sizeof status, "accepted %s.. game starting", peerName);
            back_to_idle(status);
        } else if (sequal(line, "no")) {
            send_msg(active.fd, MSG_NAH);
            snprintf(status, sizeof status, "declined %s", peerName);
            back_to_idle(status);
        } else {
            snprintf(status, sizeof status, "%s wants to play. type yes or no", peerName);
            draw_status(status);
        }
        return;
    }

    // TODO: challenge <ID> when IDLE
}
