
CC=gcc
CFLAGS=-Wall -Wextra -g
LDFLAGS=-lncurses
EXE=cli-cli

$(EXE): main.o tui.o async_read.o
	$(CC) -o $(EXE)  main.o tui.o async_read.o $(LDFLAGS)

main.o: main.c
	$(CC) -c main.c $(CFLAGS)

async_read.o: async_read.c
	$(CC) -c async_read.c $(CFLAGS)

tui.o: tui.c
	$(CC) -c tui.c $(CFLAGS)

history.o: history.c
	$(CC) -c history.c $(CFLAGS)

.PHONY: clean
clean:
	rm -f *.o $(EXE)
