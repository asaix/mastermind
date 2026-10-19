#ifndef BREAKER_H
#define BREAKER_H

// codebreaker side of the game, driven by main's poll loop

void breaker_start(int sockfd, char* opponentName);
bool breaker_input(char* line);
void breaker_socket();
int breaker_fd();

#endif
