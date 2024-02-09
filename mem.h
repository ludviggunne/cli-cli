
#ifndef MEM_H
#define MEM_H

#include <stdlib.h>

extern int returncode;
extern void cleanup(int);

static inline void * _malloc(size_t size)
{
    void * __ptr = malloc(size);

    if (__ptr == NULL)
    {
       fprintf(stderr, "error: out of memoryn");
       returncode = EXIT_FAILURE;
       cleanup(0);
    }

    return __ptr;
}

#endif
