#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "include/utils.h"
#include "include/term.h"
#include "include/comm.h"
#include "include/breaker.h"

#define STATUS_SIZE 96

enum phase {
    WAIT_SECRET,
    ENTER_GUESS,
    WAIT_FEEDBACK,
};

static int gameFd = -1; // -1 after disconnect
static char opponent[NAME_FIELD_SIZE];
static enum phase phase;

static char guesses[MAX_ATTEMPTS][SEQ_LEN + 1];
static char feedbacks[MAX_ATTEMPTS][SEQ_LEN + 1];
static int count;

static char buf[MSG_BUF_SIZE]; // reassembles split messages
static size_t len;


static void disconnected() {

    // close socket, show disconnected

    close(gameFd);
    gameFd = -1;

    char status[STATUS_SIZE];
    snprintf(status, sizeof status, "%s disconnected", opponent);
    draw_status(status);
}


static void wait_secret(char* msg) {

    // READY arrived: show the board

    if (!sequal(msg, MSG_READY)) return;

    phase = ENTER_GUESS;
    draw_board(guesses, feedbacks, count, NULL);
    draw_status("enter your guess (5 digits)");
}


static void enter_guess(char* line) {

    // validate, send and record guess

    if (count == MAX_ATTEMPTS) {
        draw_status("out of attempts (TODO: end game)");
        return;
    }

    if (!valid_seq(line, "0123456789")) {
        draw_error("a guess must be exactly 5 digits");
        return;
    }

    char msg[MSG_BUF_SIZE];
    snprintf(msg, sizeof msg, "%s%s", MSG_GUESS, line);
    if (send_msg(gameFd, msg) == -1) {
        disconnected();
        return;
    }

    snprintf(guesses[count], sizeof guesses[count], "%s", line);
    feedbacks[count][0] = '\0';
    count++;
    phase = WAIT_FEEDBACK;
    draw_board(guesses, feedbacks, count, NULL);

    char status[STATUS_SIZE];
    snprintf(status, sizeof status, "waiting for %s's feedback", opponent);
    draw_status(status);
}


static void wait_feedback(char* msg) {

    // record feedback for latest guess

    if (strncmp(msg, MSG_FEEDBACK, strlen(MSG_FEEDBACK)) != 0) return;

    // latest guess's row
    snprintf(feedbacks[count - 1], sizeof feedbacks[count - 1], "%s", msg + strlen(MSG_FEEDBACK));
    phase = ENTER_GUESS;
    draw_board(guesses, feedbacks, count, NULL);
    // TODO: reveal on xxxxx
    draw_status("enter your guess (5 digits)");
}


void breaker_start(int sockfd, char* opponentName) {

    // reset state for new game

    gameFd = sockfd;
    snprintf(opponent, sizeof opponent, "%s", opponentName);
    phase = WAIT_SECRET;
    count = 0;
    len = 0;
    buf[0] = '\0';

    char notice[STATUS_SIZE];
    snprintf(notice, sizeof notice, "waiting for %s to set the code", opponent);
    draw_notice(notice);
    draw_status("");
}


void breaker_input(char* line) {

    // handle a typed line

    line[strcspn(line, "\n")] = '\0';
    if (gameFd == -1) return;

    char status[STATUS_SIZE];
    if (phase == WAIT_SECRET) {
        snprintf(status, sizeof status, "please wait.. %s is setting the code", opponent);
        draw_error(status);
    } else if (phase == WAIT_FEEDBACK) {
        snprintf(status, sizeof status, "please wait for %s to respond to your earlier guess", opponent);
        draw_error(status);
    } else {
        enter_guess(line);
    }
}


void breaker_socket() {

    // read socket, handle complete messages

    if (gameFd == -1) return;

    if (recv_into(gameFd, buf, &len) <= 0) {
        disconnected();
        return;
    }

    char msg[MSG_BUF_SIZE];
    // stop if disconnected mid-loop
    while (gameFd != -1 && xmsg(buf, &len, msg, sizeof msg)) {
        if (phase == WAIT_SECRET) wait_secret(msg);
        else if (phase == WAIT_FEEDBACK) wait_feedback(msg);
    }
}


int breaker_fd() {

    // socket for main to poll

    // returns fd, -1 if closed

    return gameFd;
}
