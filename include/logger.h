#ifndef LOGGER_H
#define LOGGER_H

#include <stddef.h>
#include <netinet/in.h>

// log line: <TCP|UDP> <kind> <msg> <from ip:port> <to ip:port>

void log_init(bool enabled);
void log_tcp(char* kind, int sockfd, char* data, size_t n);
void log_tcp_note(char* kind, int sockfd, char* note);
void log_udp(char* kind, int sockfd, char* data, size_t n, struct sockaddr_in* other);

#endif
