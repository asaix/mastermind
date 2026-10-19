#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "include/utils.h"
#include "include/logger.h"

#define PATH_SIZE 4096
#define ESC_SIZE 512
#define ADDR_SIZE (INET_ADDRSTRLEN + 6)

static FILE* logFile = NULL; // NULL when logging is off


void log_init(bool enabled) {

    // open log.txt next to the binary

    if (!enabled) return;

    char path[PATH_SIZE];
    ssize_t n = readlink("/proc/self/exe", path, sizeof path - 1);
    if (n == -1) ewerror("logger", "log_init", "failed to find the binary's directory\n", 1);
    path[n] = '\0';

    // keep the dir, swap the filename
    char* slash = strrchr(path, '/');
    snprintf(slash + 1, sizeof path - (slash + 1 - path), "log.txt");

    logFile = fopen(path, "a");
    if (logFile == NULL) ewerror("logger", "log_init", "failed to open log.txt\n", 1);
}


static void escape(char* out, char* data, size_t n) {

    // C-style escapes, keeps lines on one line

    size_t o = 0;
    for (size_t i = 0; i < n && o < ESC_SIZE - 5; i++) {
        unsigned char c = data[i];
        if (c == '\r') o += sprintf(out + o, "\\r");
        else if (c == '\n') o += sprintf(out + o, "\\n");
        else if (c == '\\') o += sprintf(out + o, "\\\\");
        else if (c < 0x20 || c >= 0x7f) o += sprintf(out + o, "\\x%02x", c);
        else out[o++] = c;
    }
    out[o] = '\0';
}


static void addr_str(char* out, struct sockaddr_in* addr) {

    // ip:port, ? if unknown

    char ip[INET_ADDRSTRLEN];
    if (addr == NULL || inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof ip) == NULL) {
        snprintf(out, ADDR_SIZE, "?");
        return;
    }
    snprintf(out, ADDR_SIZE, "%s:%d", ip, ntohs(addr->sin_port));
}


static void write_line(char* proto, char* kind, char* msg, char* from, char* to) {

    // timestamp code from the spec

    char time_buffer[30];
    struct timeval tv;
    time_t curtime;

    gettimeofday(&tv, NULL);
    curtime = tv.tv_sec;
    strftime(time_buffer, 30, "%Y-%m-%d %H:%M:%S", localtime(&curtime));

    fprintf(logFile, "[%s.%06ld] [LOG] %s %s %s %s -> %s\n", time_buffer, tv.tv_usec, proto, kind, msg, from, to);
    fflush(logFile);
}


static void tcp_line(char* kind, int sockfd, char* msg) {

    // both ends from the fd

    struct sockaddr_in local, peer;
    socklen_t len = sizeof local;
    bool haveLocal = getsockname(sockfd, (struct sockaddr*) &local, &len) == 0;
    len = sizeof peer;
    bool havePeer = getpeername(sockfd, (struct sockaddr*) &peer, &len) == 0;

    char localStr[ADDR_SIZE], peerStr[ADDR_SIZE];
    addr_str(localStr, haveLocal ? &local : NULL);
    addr_str(peerStr, havePeer ? &peer : NULL);

    // incoming kinds flow peer -> us
    bool incoming = sequal(kind, "RECV") || sequal(kind, "ACCEPT");
    if (incoming) write_line("TCP", kind, msg, peerStr, localStr);
    else write_line("TCP", kind, msg, localStr, peerStr);
}


void log_tcp(char* kind, int sockfd, char* data, size_t n) {

    // a message on a tcp socket

    if (logFile == NULL) return;

    char esc[ESC_SIZE];
    escape(esc, data, n);
    tcp_line(kind, sockfd, esc);
}


void log_tcp_note(char* kind, int sockfd, char* note) {

    // connect/accept/close/EOF, note is "-" or "<EOF>"

    if (logFile == NULL) return;
    tcp_line(kind, sockfd, note);
}


void log_udp(char* kind, int sockfd, char* data, size_t n, struct sockaddr_in* other) {

    // a discovery datagram, other is its sender or destination

    if (logFile == NULL) return;

    struct sockaddr_in local;
    socklen_t len = sizeof local;
    bool haveLocal = getsockname(sockfd, (struct sockaddr*) &local, &len) == 0;

    char esc[ESC_SIZE], localStr[ADDR_SIZE], otherStr[ADDR_SIZE];
    escape(esc, data, n);
    addr_str(localStr, haveLocal ? &local : NULL);
    addr_str(otherStr, other);

    if (sequal(kind, "RECV")) write_line("UDP", kind, esc, otherStr, localStr);
    else write_line("UDP", kind, esc, localStr, otherStr);
}
