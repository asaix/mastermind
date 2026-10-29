CC = gcc
CFLAGS = -std=c23 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 \
         -Wall -Wextra -Werror -Wno-unused-parameter -g
LDLIBS =

PREFIX = $(HOME)/.local
BINDIR = $(PREFIX)/bin

SRCS = $(wildcard *.c)

mastermind: $(SRCS)
	$(CC) $(CFLAGS) -o mastermind $(SRCS) $(LDLIBS)

install: mastermind
	mkdir -p $(DESTDIR)$(BINDIR)
	install -m 755 mastermind $(DESTDIR)$(BINDIR)/mastermind

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/mastermind

clean:
	rm -f mastermind

.PHONY: install uninstall clean
