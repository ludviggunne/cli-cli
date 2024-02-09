
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "async_read.h"

struct async_read async_read_create (FILE * stream)
{
    struct async_read rd;
    rd.stream = stream;
    rd.line = NULL;
    rd.internal = NULL;
    rd.done = 1;
    pthread_mutex_init(&rd.lock, NULL);
    return rd;
}

char * async_read_line (struct async_read * rd)
{
    pthread_mutex_lock(&rd->lock);
    char * line = rd->line;
    // let reader know that we consumed the line
    rd->line = NULL;
    pthread_mutex_unlock(&rd->lock);
    return line;
}

static void * _read_task (void * data)
{
    size_t _size = 0;
    int read;
    int waiting;
    int _oldstate;

    struct async_read * rd = data;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &_oldstate);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &_oldstate);

    while (!rd->done)
    {
        read = getline(&rd->internal, &_size, rd->stream);
        if (read > -1)
        {
            // wait for main thread to consume previous line
            waiting = 1;
            while (waiting)
            {
                // don't want to cancel while we're locking
                pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &_oldstate);
                pthread_mutex_lock(&rd->lock);
                if (rd->line == NULL)
                {
                    // duplicate internal string, in case getline() messes with it
                    // TODO: custom strdup with reallocation
                    rd->line = strdup(rd->internal);
                    waiting = 0;
                }
                pthread_mutex_unlock(&rd->lock);
                pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &_oldstate);
            }
        }
        else
        {
            break;
        }
    }

    return NULL;
}

void async_read_start (struct async_read * rd)
{
    rd->done = 0;
    pthread_create(&rd->thread, NULL, _read_task, rd);
}

void async_read_stop (struct async_read * rd)
{
    // cancelation required since getline() may block indefinitely
    pthread_cancel(rd->thread);
    pthread_join(rd->thread, NULL);

    if (rd->line != NULL)
    {
        free(rd->line);
        rd->line = NULL;
    }

    if (rd->internal != NULL)
    {
        free(rd->internal);
        rd->internal = NULL;
    }

    if (rd->done) return;
}
