#ifndef TERM_H
#define TERM_H

char* name_entry();

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
