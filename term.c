#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "include/term.h"
#include "include/comm.h"

#define TITLE_ROW 1
#define HEADER_ROW 2
#define PEERS_FIRST_ROW 3
#define DIVIDER_ROW (PEERS_FIRST_ROW + MAX_PEERS)

// game board: one row per attempt, then a divider, then the master sequence
#define BOARD_FIRST_ROW 1
#define BOARD_DIVIDER_ROW (BOARD_FIRST_ROW + MAX_ATTEMPTS)
#define BOARD_SECRET_ROW (BOARD_DIVIDER_ROW + 1)

// fixed 2 rows below the board (taller than the peer table), so the prompt doesn't move between screens
#define PROMPT_ROW (BOARD_SECRET_ROW + 2)
#define STATUS_ROW (PROMPT_ROW + 1)

#define STATUS_SIZE 128

// store last status  so draw_prompt can put it back after clearing
static char status[STATUS_SIZE] = "";
static bool statusIsError = false; // red instead of cyan

char* name_entry() {
    char* name = malloc(MAX_NAME_SIZE * 2 * sizeof(char));

    fputs("Enter your name: ", stdout);
    fgets(name, NAME_FIELD_SIZE, stdin); // MAX_NAME_SIZE chars + \0

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

static void print_status() {
    if (statusIsError) red(status);
    else cyan(status);
    rst();
}

static void show_status(char* msg, bool isError) {

    snprintf(status, sizeof status, "%s", msg);
    statusIsError = isError;

    hidecur();
    savecur();

    jrc(STATUS_ROW, 1);
    print_status();
    cc2eol();

    restorecur();
    showcur();
}

void draw_status(char* msg) {
    show_status(msg, false);
}

void draw_error(char* msg) {
    show_status(msg, true);
}

void draw_notice(char* msg) {

    // clear the board area and show msg on its first row instead

    hidecur();
    savecur();

    for (int row = BOARD_FIRST_ROW; row <= BOARD_SECRET_ROW; row++) {
        jrc(row, 1);
        cc2eol();
    }
    jrc(BOARD_FIRST_ROW, 1);
    fputs(msg, stdout);

    restorecur();
    showcur();
}

void draw_prompt() {

    // fresh prompt on the fixed prompt row, clears anything below it
    // then puts the status line back under it
    // leaves the cursor after "> "

    jrc(PROMPT_ROW, 1);
    cc2eos();

    jrc(STATUS_ROW, 1);
    print_status();

    jrc(PROMPT_ROW, 1);
    fputs("> ", stdout);
    fflush(stdout);
}


static void draw_feedback(char* fb) {

    // x green, o yellow, - red

    for (int i = 0; fb[i] != '\0'; i++) {
        char c[2] = { fb[i], '\0' };
        if (fb[i] == 'x') green(c);
        else if (fb[i] == 'o') yellow(c);
        else red(c);
    }
    rst();
}

void draw_board(char guesses[][SEQ_LEN + 1], char feedback[][SEQ_LEN + 1], int count, char* secret) {

    // +1 for \0
    
    // redraw the game board in place above the prompt
    // rows past count show *****, a guess still waiting for feedback has a blank feedback column
    // secret NULL or "" shows ***** (codebreaker's view, or before it's set)

    hidecur();
    savecur();

    for (int i = 0; i < MAX_ATTEMPTS; i++) {
        jrc(BOARD_FIRST_ROW + i, 1);
        if (i < count) {
            printf("%s  ", guesses[i]);
            draw_feedback(feedback[i]);
        } else {
            fputs("*****  *****", stdout);
        }
        cc2eol();
    }

    jrc(BOARD_DIVIDER_ROW, 1);
    fputs("-----", stdout);
    cc2eol();

    jrc(BOARD_SECRET_ROW, 1);
    fputs((secret != NULL && secret[0] != '\0') ? secret : "*****", stdout);
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
