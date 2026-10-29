<h1 align="center">mastermind</h1>

<p align="center">Two-player Mastermind in your terminal, played over a LAN.</p>

---

mastermind is a program for playing [Mastermind](https://en.wikipedia.org/wiki/Mastermind_(board_game)) against someone else on
your network. Start it on two machines and they find each other on their own.

```
┏━━━━━━━━━━━━━━━━━━━━━━ MASTERMIND ━━━━━━━━━━━━━━━━━━━━━━━┓
┃ ID  NAME            IP              PORT      LAST SEEN ┃
┠─────────────────────────────────────────────────────────┨
┃ 1   bob             10.94.78.196    7001         1s ago ┃
┃ 2   carol           10.94.78.12     7000         0s ago ┃
┃ 3   dave            10.94.78.203    7000         2s ago ┃
┃ 4   erin            10.94.78.57     7000         1s ago ┃
┃ 5   erin2           10.94.78.57     7001         3s ago ┃
┃                                                         ┃
┃                                                         ┃
┃                                                         ┃
┃                                                         ┃
┃                                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━ 5 online ━━━━━━━━━━━━━━━━━━━━━━━━┛

                 ❯ challenge 2
                   challenge <ID>
```


## Gameplay

```
mastermind [--log]
```

- `--log`: log every message sent and received to `log.txt`.

Pick a name, then challenge someone from the lobby with `challenge <ID>`. If
they accept, you set a secret 5-digit code and they get 12 guesses to find it.
You score each guess yourself: 
- `x` for right digit in the right place
- `o` for
right digit in the wrong place
- `-` for not in the code.

```
      mastermind (alice)                  codebreaker (bob)

┏━━━━┯━━━━━━━━━━━┯━━━━━━━━━━━┓      ┏━━━━┯━━━━━━━━━━━┯━━━━━━━━━━━┓
┃  # │   GUESS   │ FEEDBACK  ┃      ┃  # │   GUESS   │ FEEDBACK  ┃
┣━━━━┿━━━━━━━━━━━┿━━━━━━━━━━━┫      ┣━━━━┿━━━━━━━━━━━┿━━━━━━━━━━━┫
┃ 01 │ 1 2 3 4 5 │ o o o o - ┃      ┃ 01 │ 1 2 3 4 5 │ o o o o - ┃
┃ 02 │ 4 1 2 0 3 │ x o x o x ┃      ┃ 02 │ 4 1 2 0 3 │ x o x o x ┃
┃ 03 │ 4 0 2 3 1 │           ┃      ┃ 03 │ 4 0 2 3 1 │           ┃
┃ 04 │ · · · · · │ · · · · · ┃      ┃ 04 │ · · · · · │ · · · · · ┃
┃ 05 │ · · · · · │ · · · · · ┃      ┃ 05 │ · · · · · │ · · · · · ┃
┃ 06 │ · · · · · │ · · · · · ┃      ┃ 06 │ · · · · · │ · · · · · ┃
┃ 07 │ · · · · · │ · · · · · ┃      ┃ 07 │ · · · · · │ · · · · · ┃
┃ 08 │ · · · · · │ · · · · · ┃      ┃ 08 │ · · · · · │ · · · · · ┃
┃ 09 │ · · · · · │ · · · · · ┃      ┃ 09 │ · · · · · │ · · · · · ┃
┃ 10 │ · · · · · │ · · · · · ┃      ┃ 10 │ · · · · · │ · · · · · ┃
┃ 11 │ · · · · · │ · · · · · ┃      ┃ 11 │ · · · · · │ · · · · · ┃
┃ 12 │ · · · · · │ · · · · · ┃      ┃ 12 │ · · · · · │ · · · · · ┃
┣━━━━┷━━━━━━━━━━━┷━━━━━━━━━━━┫      ┣━━━━┷━━━━━━━━━━━┷━━━━━━━━━━━┫
┃      CODE  4 0 2 1 3       ┃      ┃      CODE  ? ? ? ? ?       ┃
┗━━━━━━━━ MASTERMIND ━━━━━━━━┛      ┗━━━━━━━━ MASTERMIND ━━━━━━━━┛

  ❯                                   ❯

  feedback for 40231 (x o -)         waiting for alice's feedback
```

*Just like the real game, the mastermind enters the feedback manually, so they might lie. Be careful!*


## Installing

### From a release

Download `mastermind-linux-x86_64.tar.gz` from the [latest release](../../releases/latest),
then unpack it and run the install script, which copies `mastermind` to
`~/.local/bin`:

```sh
tar -xzf mastermind-linux-x86_64.tar.gz
./mastermind-linux-x86_64/install.sh
```

To remove it, run `./mastermind-linux-x86_64/uninstall.sh`.

### From source

```sh
make install      # builds mastermind and installs it to ~/.local/bin
make uninstall    # removes it
```

This installs for your user and doesn't need `sudo`. To install somewhere
else, set `PREFIX` on both commands, e.g. `sudo make install PREFIX=/usr/local`.


## Building

You need GCC 14+ (for C23) on Linux.

```sh
make          # builds ./mastermind
make clean    # removes it
```


## Implementation

![mastermind state diagram](assets/state.svg)

Players find each other by broadcasting a UDP packet every 2 seconds on port
9555:

```
+-------+------------+-------------------------+------------------+
| LIES  | session id |          name           |       port       |
|  4 B  |  2 B (BE)  | 16 B, PKCS#7 padded     | ASCII digits     |
+-------+------------+-------------------------+------------------+
```

The challenge and the game itself go over TCP. Messages end with `\r\n\r\n`:

```
B -> A   WHO ARE YOU
A -> B   <name>\r\n<port>
B -> A   OK | NAH | BUSY

A -> B   READY
B -> A   GUESS:<guess>
A -> B   FEEDBACK:<feedback>
...
A -> B   REVEAL:<code>
```


## Exit codes

| Code | Meaning |
| ---- | ------- |
| 1 | General error |
| 2 | Network error |


## License

This program is licensed under the [MIT License](LICENSE).

## Contributions

Contributions are welcome. Please submit a PR or issue on Github.

## AI usage

AI was used to generate the implementation diagram and a few quality of life changes.