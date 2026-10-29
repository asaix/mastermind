#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "include/term.h"
#include "include/comm.h"

// lobby: heavy box, title in the top border, one row per peer, count in the bottom border
#define LOBBY_INNER 57
#define LOBBY_WIDTH (LOBBY_INNER + 2)
#define LOBBY_TOP_ROW 1
#define HEADER_ROW 2
#define RULE_ROW 3
#define PEERS_FIRST_ROW 4
#define LOBBY_BOTTOM_ROW (PEERS_FIRST_ROW + MAX_PEERS)

// game board: # | guess | feedback columns, one row per attempt, then the master sequence
#define NUM_W 4
#define SEQ_W (SEQ_LEN * 2 + 1) // digits spaced out, 1 pad each side
#define BOARD_INNER (NUM_W + 1 + SEQ_W + 1 + SEQ_W)
#define BOARD_WIDTH (BOARD_INNER + 2)
#define BOARD_TOP_ROW 1
#define BOARD_HEADER_ROW 2
#define BOARD_RULE_ROW 3
#define BOARD_FIRST_ROW 4
#define BOARD_DIVIDER_ROW (BOARD_FIRST_ROW + MAX_ATTEMPTS)
#define BOARD_SECRET_ROW (BOARD_DIVIDER_ROW + 1)
#define BOARD_BOTTOM_ROW (BOARD_SECRET_ROW + 1)

// fixed 2 rows below the board (taller than the lobby), so the prompt doesn't move between screens
#define PROMPT_ROW (BOARD_BOTTOM_ROW + 2)
#define STATUS_ROW (PROMPT_ROW + 1)

#define STATUS_SIZE 128

// store last status  so draw_prompt can put it back after clearing
static char status[STATUS_SIZE] = "";
static bool statusIsError = false; // red instead of cyan


static int left_col(int width) {

    // column that centers something width wide, 80 cols if the size is unknown

    struct winsize ws;
    int cols = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) cols = ws.ws_col;

    int col = (cols - width) / 2 + 1;
    return col < 1 ? 1 : col;
}

static void row_at(int row, int col) {
    // wipe the whole row, then jump to col on it
    jrc(row, 1);
    cc2eol();
    jrc(row, col);
}

static void frame(char* s) {
    // box drawing, always cyan
    cyan(s);
    rst();
}

static void repeat(char* s, int n) {
    for (int i = 0; i < n; i++) fputs(s, stdout);
}

static void border(char* left, char* right, int inner, char* label) {

    // heavy horizontal line with label centered in it, label may be NULL

    int labelLen = (label != NULL) ? strlen(label) : 0;
    int before = (inner - labelLen) / 2;

    cyan(left);
    repeat("━", before);
    rst();
    if (label != NULL) bold(label);
    rst();
    cyan("");
    repeat("━", inner - before - labelLen);
    cyan(right);
    rst();
}

static void board_rule(char* left, char* join, char* right) {

    // heavy horizontal line split at the board's column edges

    cyan(left);
    repeat("━", NUM_W);
    fputs(join, stdout);
    repeat("━", SEQ_W);
    fputs(join, stdout);
    repeat("━", SEQ_W);
    fputs(right, stdout);
    rst();
}

static void spaced(char* s, int len) {

    // " a b c d e ", short strings padded with spaces so the columns stay lined up

    putchar(' ');
    for (int i = 0; i < len; i++) {
        char c = (i < (int) strlen(s)) ? s[i] : ' ';
        printf("%c ", c);
    }
}


char* name_entry() {
    char* name = malloc(MAX_NAME_SIZE * 2 * sizeof(char));

    cls();
    int col = left_col(BOARD_WIDTH);

    row_at(8, col);
    border("┏", "┓", BOARD_INNER, " MASTERMIND ");
    row_at(9, col);
    frame("┃");
    char* hint = "pick a name (15 chars max)";
    int pad = (BOARD_INNER - (int) strlen(hint)) / 2;
    repeat(" ", pad);
    dim(hint);
    rst();
    repeat(" ", BOARD_INNER - pad - (int) strlen(hint));
    frame("┃");
    row_at(10, col);
    border("┗", "┛", BOARD_INNER, NULL);

    jrc(12, col + 2);
    bold("");
    cyan("❯ ");
    rst();
    fflush(stdout);
    fgets(name, NAME_FIELD_SIZE, stdin); // MAX_NAME_SIZE chars + \0

    // drop trailing newline if present
    size_t newlineIndex = strcspn(name, "\n");
    name[newlineIndex] = '\0';

    return name;

}

void draw_peers(struct peer* peers, int peerCount) {

    time_t now = time(NULL);
    int col = left_col(LOBBY_WIDTH);

    hidecur();
    savecur();

    row_at(LOBBY_TOP_ROW, col);
    border("┏", "┓", LOBBY_INNER, " MASTERMIND ");
    cc2eol();

    char header[LOBBY_INNER + 1];
    snprintf(header, sizeof header, " %-3s %-15s %-15s %-6s %12s ", "ID", "NAME", "IP", "PORT", "LAST SEEN");
    row_at(HEADER_ROW, col);
    frame("┃");
    bold(header);
    rst();
    frame("┃");
    cc2eol();

    row_at(RULE_ROW, col);
    frame("┠");
    cyan("");
    repeat("─", LOBBY_INNER);
    rst();
    frame("┨");
    cc2eol();

    for (int i = 0; i < MAX_PEERS; i++) {
        // jump to next row
        row_at(PEERS_FIRST_ROW + i, col);
        frame("┃");

        // redraw row
        // if peer was removed this blanks the whole entry
        if (i < peerCount) {
            char id[8], name[NAME_FIELD_SIZE + 2], rest[48], ago[24], seen[32];
            snprintf(id, sizeof id, " %-3d ", i + 1);
            snprintf(name, sizeof name, "%-15s ", peers[i].name);
            snprintf(rest, sizeof rest, "%-15s %-6d ", peers[i].ip, peers[i].port);
            snprintf(ago, sizeof ago, "%lds ago", (long) (now - peers[i].lastSeen));
            snprintf(seen, sizeof seen, "%12s ", ago);
            bold(""); cyan(id); rst();
            bold(name); rst();
            fputs(rest, stdout);
            dim(seen); rst();
        } else if (i == 0) {
            char* waiting = "waiting for players on your network...";
            int pad = (LOBBY_INNER - (int) strlen(waiting)) / 2;
            repeat(" ", pad);
            dim(waiting);
            rst();
            repeat(" ", LOBBY_INNER - pad - (int) strlen(waiting));
        } else {
            repeat(" ", LOBBY_INNER);
        }

        frame("┃");
        cc2eol();
    }

    char count[24];
    snprintf(count, sizeof count, " %d online ", peerCount);
    row_at(LOBBY_BOTTOM_ROW, col);
    border("┗", "┛", LOBBY_INNER, count);
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

    // centered under the board
    jrc(STATUS_ROW, 1);
    cc2eol();
    jrc(STATUS_ROW, left_col(strlen(status)));
    print_status();

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

    // clear the board area and show msg in a small box in the middle of it instead

    hidecur();
    savecur();

    for (int row = BOARD_TOP_ROW; row <= BOARD_BOTTOM_ROW; row++) {
        jrc(row, 1);
        cc2eol();
    }

    int inner = strlen(msg) + 4;
    int col = left_col(inner + 2);
    int row = (BOARD_TOP_ROW + BOARD_BOTTOM_ROW) / 2 - 1;

    row_at(row, col);
    border("┏", "┓", inner, NULL);
    row_at(row + 1, col);
    frame("┃");
    printf("  %s  ", msg);
    frame("┃");
    row_at(row + 2, col);
    border("┗", "┛", inner, NULL);

    restorecur();
    showcur();
}

void draw_prompt() {

    // fresh prompt on the fixed prompt row, clears anything below it
    // then puts the status line back under it
    // leaves the cursor after the prompt, lined up with the board's left edge

    jrc(PROMPT_ROW, 1);
    cc2eos();

    jrc(STATUS_ROW, left_col(strlen(status)));
    print_status();

    jrc(PROMPT_ROW, left_col(BOARD_WIDTH) + 2);
    bold("");
    cyan("❯ ");
    rst();
    fflush(stdout);
}


static void draw_feedback(char* fb) {

    // " x o - o x ", x green, o yellow, - red

    putchar(' ');
    for (int i = 0; i < SEQ_LEN; i++) {
        char c[3] = { (i < (int) strlen(fb)) ? fb[i] : ' ', ' ', '\0' };
        bold("");
        if (c[0] == 'x') green(c);
        else if (c[0] == 'o') yellow(c);
        else red(c);
        rst();
    }
}

void draw_board(char guesses[][SEQ_LEN + 1], char feedback[][SEQ_LEN + 1], int count, char* secret) {

    // +1 for \0

    // redraw the game board in place above the prompt
    // rows past count are dotted out, a guess still waiting for feedback has a blank feedback column
    // secret NULL or "" shows ? ? ? ? ? (codebreaker's view, or before it's set)

    int col = left_col(BOARD_WIDTH);

    hidecur();
    savecur();

    row_at(BOARD_TOP_ROW, col);
    board_rule("┏", "┯", "┓");
    cc2eol();

    row_at(BOARD_HEADER_ROW, col);
    frame("┃");
    bold("  # ");
    frame("│");
    bold("   GUESS   ");
    frame("│");
    bold(" FEEDBACK  ");
    frame("┃");
    cc2eol();

    row_at(BOARD_RULE_ROW, col);
    board_rule("┣", "┿", "┫");
    cc2eol();

    for (int i = 0; i < MAX_ATTEMPTS; i++) {
        row_at(BOARD_FIRST_ROW + i, col);
        frame("┃");

        char num[8];
        snprintf(num, sizeof num, " %02d ", i + 1);

        if (i < count) {
            fputs(num, stdout);
            frame("│");
            bold("");
            spaced(guesses[i], SEQ_LEN);
            rst();
            frame("│");
            if (feedback[i][0] != '\0') draw_feedback(feedback[i]);
            else repeat(" ", SEQ_W);
        } else {
            dim(num);
            rst();
            frame("│");
            dim(" · · · · · ");
            rst();
            frame("│");
            dim(" · · · · · ");
            rst();
        }

        frame("┃");
        cc2eol();
    }

    row_at(BOARD_DIVIDER_ROW, col);
    board_rule("┣", "┷", "┫");
    cc2eol();

    // "CODE  4 0 2 1 3" centered in the row
    int codeLen = 4 + 2 + SEQ_LEN * 2 - 1;
    int pad = (BOARD_INNER - codeLen) / 2;
    row_at(BOARD_SECRET_ROW, col);
    frame("┃");
    repeat(" ", pad);
    dim("CODE ");
    rst();
    if (secret != NULL && secret[0] != '\0') {
        bold("");
        spaced(secret, SEQ_LEN);
    } else {
        dim(" ? ? ? ? ? ");
    }
    rst();
    repeat(" ", BOARD_INNER - pad - codeLen - 1);
    frame("┃");
    cc2eol();

    row_at(BOARD_BOTTOM_ROW, col);
    border("┗", "┛", BOARD_INNER, " MASTERMIND ");
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
