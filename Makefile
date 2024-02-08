
CC=gcc
CFLAGS=-Wall -Wextra -g
LDFLAGS=-lncurses
EXE=cli-cli

$(EXE): main.o async_reader.o tui.o
	$(CC) -o $(EXE)  main.o async_reader.o tui.o $(LDFLAGS)

main.o: main.c
	$(CC) -c main.c $(CFLAGS)

async_reader.o: async_reader.c
	$(CC) -c async_reader.c $(CFLAGS)

tui.o: tui.c
	$(CC) -c tui.c $(CFLAGS)

.PHONY: clean
clean:
	rm -f *.o $(EXE)
