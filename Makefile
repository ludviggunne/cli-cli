
CC=gcc
CFLAGS=-Wall -Wextra -g
EXE=cli-cli

$(EXE): main.o async_reader.o
	$(CC) -o $(EXE) main.o async_reader.o

main.o: main.c
	$(CC) -c main.c $(CFLAGS)

async_reader.o: async_reader.c
	$(CC) -c async_reader.c $(CFLAGS)

.PHONY: clean
clean:
	rm -f *.o $(EXE)
