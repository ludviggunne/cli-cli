
#ifndef READER_H
#define READER_H

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

struct async_read {
    FILE * stream;
    char * line;
    char * internal;
    int done;
    pthread_t thread;
    pthread_mutex_t lock;
};

// creates an async reader from open stream
struct async_read async_read_create (FILE * stream);

// polls reader for a new line of input
// returns NULL if there is no new line
char * async_read_line (struct async_read * rd);

// start reader thread
void async_read_start (struct async_read * rd);

// terminate reader thread and free allocated memory
// does not close the stream
void async_read_stop (struct async_read * rd);

#endif
