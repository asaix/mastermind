#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <poll.h>

bool sequal(char* s1, char*s2);
[[noreturn]] void ewerror(char* module, char* fn, char* msg, int exitCode);
int pfd_add(struct pollfd** pfds, size_t* cap, int* n, int fd, short events);

#endif