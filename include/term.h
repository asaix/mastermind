#ifndef TERM_H
#define TERM_H

#define MAX_NAME_SIZE 15
#define SEQ_LEN 5 
#define MAX_ATTEMPTS 12

struct peer; // in comm.h

char* name_entry();
void draw_peers(struct peer* peers, int peerCount);
void draw_prompt();
void draw_status(char* msg);
void draw_board(char guesses[][SEQ_LEN + 1], char feedback[][SEQ_LEN + 1], int count, char* secret);

// screen / cursor control
void cls();
void ctl();
void jrc(int row, int col);
void cc2eol();
void cc2eos();
void savecur();
void restorecur();
void bell();
void showcur();
void hidecur();

// styled text
void green(char *s);
void yellow(char *s);
void red(char *s);
void cyan(char *s);
void bold(char *s);
void dim(char *s);
void rst();

#endif
