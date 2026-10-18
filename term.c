#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "include/term.h"
#include "include/comm.h"

// home screen layout, the prompt row stays fixed below a MAX_PEERS tall table
#define TITLE_ROW 1
#define HEADER_ROW 2
#define PEERS_FIRST_ROW 3
#define DIVIDER_ROW (PEERS_FIRST_ROW + MAX_PEERS)
#define PROMPT_ROW (DIVIDER_ROW + 1)

char* name_entry() {
    char* name = malloc(MAX_NAME_SIZE * 2 * sizeof(char));

    fputs("Enter your name: ", stdout);
    fgets(name, MAX_NAME_SIZE + 1, stdin); // +1 for \0

    // drop trailing newline if present
    size_t newlineIndex = strcspn(name, "\n");
    name[newlineIndex] = '\0';

    return name;

}

void draw_peers(struct peer* peers, int peerCount) {

    time_t now = time(NULL);

    hidecur();
    savecur();

    jrc(TITLE_ROW, 1);
    bold("Players Online:");
    rst();
    cc2eol();

    char header[64];
    snprintf(header, sizeof header, "%-4s %-15s %-15s %-7s %s", "ID", "Name", "IP Addr", "Port", "Last Seen");
    jrc(HEADER_ROW, 1);
    bold(header);
    rst();
    cc2eol();


    for (int i = 0; i < MAX_PEERS; i++) {
        // jump to next row
        jrc(PEERS_FIRST_ROW + i, 1);

        // redraw row
        if (i < peerCount) {
            printf("%-4d %-15s %-15s %-7d %ld s. ago",
                i + 1, peers[i].name, peers[i].ip, peers[i].port, (long) (now - peers[i].lastSeen));
        }
        // clear to end
        // if peer was removed this clears the whole entry
        cc2eol();
    }

    jrc(DIVIDER_ROW, 1);
    dim("________________________________________________________");
    rst();
    cc2eol();

    restorecur();
    showcur();
}

void cls() {
    // clear screen
    printf("\x1b[2J");
    fflush(stdout);
}

void ctl() {
    // cursor top left
    printf("\x1b[H");
    fflush(stdout);
}

void jrc(int row, int col) {
    // jump row column
    printf("\x1b[%d;%dH", row, col);
    fflush(stdout);
}

void cc2eol() {
    // clear from cursor to EOL
    printf("\x1b[K");
    fflush(stdout);
}

void cc2eos() {
    // clear from cursor to EOS
    printf("\x1b[J");
    fflush(stdout);
}

void savecur() {
    // save cursor 
    printf("\x1b" "7");
    fflush(stdout);
}

void restorecur() {
    // restore cursor
    printf("\x1b" "8");
    fflush(stdout);

}

void bell() {
    // bell sound
    printf("\a");
    fflush(stdout);
}

void showcur() {
    // show cursor
    printf("\x1b[?25h");
    fflush(stdout);
}

void hidecur() {
    // hide cursor
    printf("\x1b[?25l");
    fflush(stdout);
}

void green(char *s) {
    // green text
    printf("\x1b[32m%s", s);
    fflush(stdout);
}

void yellow(char *s) {
    // yellow text
    printf("\x1b[33m%s", s);
    fflush(stdout);
}

void red(char *s) {
    // red text
    printf("\x1b[31m%s", s);
    fflush(stdout);
}

void cyan(char *s) {
    // cyan text
    printf("\x1b[36m%s", s);
    fflush(stdout);
}

void bold(char *s) {
    // bold text
    printf("\x1b[1m%s", s);
    fflush(stdout);
}

void dim(char *s) {
    // dim text
    printf("\x1b[2m%s", s);
    fflush(stdout);
}

void rst() {
    // reset text attributes
    printf("\x1b[0m");
    fflush(stdout);
}
