#ifndef TERM_H
#define TERM_H

#define MAX_NAME_SIZE 15

struct peer; // in comm.h

char* name_entry();
void draw_peers(struct peer* peers, int peerCount);
void draw_prompt();

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
