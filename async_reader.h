
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

struct reader reader_create (FILE * stream);
char * reader_getline (struct reader * rd);
void reader_start (struct reader * rd);
void reader_stop (struct reader * rd);
