#ifndef MASTER_H
#define MASTER_H

// mastermind side of the game, driven by main's poll loop

void master_start(int sockfd, char* opponentName);
bool master_input(char* line);
void master_socket();
int master_fd();

#endif
