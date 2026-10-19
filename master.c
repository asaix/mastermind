#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "include/utils.h"
#include "include/term.h"
#include "include/comm.h"
#include "include/master.h"

#define STATUS_SIZE 96

enum phase {
    SECRET,   // waiting for master to enter the master sequence
    GUESS,    // waiting for a guess from the codebreaker
    FEEDBACK, // waiting for us to type feedback for the latest guess
};

static int gameFd = -1; // -1 once the codebreaker has disconnected
static char opponent[NAME_FIELD_SIZE];
static enum phase phase;

static char secret[SEQ_LEN + 1];
static char guesses[MAX_ATTEMPTS][SEQ_LEN + 1];
static char feedbacks[MAX_ATTEMPTS][SEQ_LEN + 1];
static int count; // guesses received

// bytes from codebreaker, may be in disconnected byte stream
static char buf[MSG_BUF_SIZE];
static size_t len;


static void disconnected() {
    close(gameFd);
    gameFd = -1;

    char status[STATUS_SIZE];
    snprintf(status, sizeof status, "%s disconnected", opponent);
    draw_status(status);
}


static void waiting_status() {
    char status[STATUS_SIZE];
    snprintf(status, sizeof status, "waiting for %s's guess", opponent);
    draw_status(status);
}


static void handle_guesses() {

    // take the next complete guess out of buf, if one has arrived and we're waiting for it

    char msg[MSG_BUF_SIZE];
    while (phase == GUESS && xmsg(buf, &len, msg, sizeof msg)) {
        // check if it starts w GUESS:
        if (strncmp(msg, MSG_GUESS, strlen(MSG_GUESS)) != 0) continue;

        if (count == MAX_ATTEMPTS) {
            draw_status("out of attempts (TODO: end game)");
            continue;
        }

        snprintf(guesses[count], sizeof guesses[count], "%s", msg + strlen(MSG_GUESS));
        feedbacks[count][0] = '\0';
        count++;
        phase = FEEDBACK;

        draw_board(guesses, feedbacks, count, secret);
        char status[STATUS_SIZE];
        snprintf(status, sizeof status, "feedback for %s (x o -)", guesses[count - 1]);
        draw_status(status);
    }
}


static void set_secret(char* line) {

    if (!valid_seq(line, "0123456789")) {
        draw_status("the master sequence must be exactly 5 digits");
        return;
    }

    snprintf(secret, sizeof secret, "%s", line);

    // tell the codebreaker it can start guessing
    if (send_msg(gameFd, MSG_READY) == -1) {
        disconnected();
        return;
    }

    phase = GUESS;
    draw_board(guesses, feedbacks, count, secret);
    waiting_status();
}


static void wait_guess() {

    // the socket is readable: buffer what came in, handle it if we're waiting for a guess

    if (recv_into(gameFd, buf, &len) <= 0) {
        disconnected();
        return;
    }
    handle_guesses();
}


static void feedback(char* line) {

    if (!valid_seq(line, "xo-")) {
        draw_status("feedback must be exactly 5 of x o -");
        return;
    }

    char msg[MSG_BUF_SIZE];
    snprintf(msg, sizeof msg, "%s%s", MSG_FEEDBACK, line);
    if (send_msg(gameFd, msg) == -1) {
        disconnected();
        return;
    }

    snprintf(feedbacks[count - 1], sizeof feedbacks[count - 1], "%s", line);
    phase = GUESS;
    draw_board(guesses, feedbacks, count, secret);
    waiting_status();

    // TODO: reveal() when line is xxxxx or count == MAX_ATTEMPTS
}


static void reveal() {

    // only at the end of the game, the master sequence never goes out before this

    char msg[MSG_BUF_SIZE];
    snprintf(msg, sizeof msg, "%s%s", MSG_REVEAL, secret);
    if (send_msg(gameFd, msg) == -1) disconnected();
}


void master_start(int sockfd, char* opponentName) {

    // fresh game against opponentName on sockfd, we're the mastermind

    // IMP: SET GAMEFD HERE
    gameFd = sockfd;
    snprintf(opponent, sizeof opponent, "%s", opponentName);
    phase = SECRET;
    secret[0] = '\0';
    count = 0;
    len = 0;
    buf[0] = '\0';

    draw_board(guesses, feedbacks, count, secret);
    draw_status("enter the master sequence (5 digits)");
}


void master_input(char* line) {

    // a line typed at the prompt during the game

    line[strcspn(line, "\n")] = '\0';
    if (gameFd == -1) return;

    if (phase == SECRET) set_secret(line);
    else if (phase == FEEDBACK) feedback(line);
    else waiting_status();
}


void master_socket() {

    // poll reported the game socket

    if (gameFd != -1) wait_guess();
}


int master_fd() {

    // the socket main should poll, -1 if there's nothing to poll

    return gameFd;
}
