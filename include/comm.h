#ifndef COMM_H
#define COMM_H

int bind_game_port(int increment);
int get_bound_port(int sockfd);

// discovery
int init_discovery_sock();
int hello_i_exist(int sockfd, char* name, int port);

#endif
