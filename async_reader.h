
#ifndef READER_H
#define READER_H

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

struct reader {
    FILE * stream;
    char * line;
    char * internal;
    int done;
    pthread_t thread;
    pthread_mutex_t lock;
};

// creates an async reader from open stream
struct reader reader_create (FILE * stream);

// polls reader for a new line of input
// returns NULL if there is no new line
char * reader_getline (struct reader * rd);

// start reader thread
void reader_start (struct reader * rd);

// terminate reader thread and free allocated memory
// does not close the stream
void reader_stop (struct reader * rd);

#endif
