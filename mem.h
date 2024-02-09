
#ifndef MEM_H
#define MEM_H

#include <stdlib.h>

#define malloc(x)\
    ({\
     void * __ptr = malloc(x);\
     if (__ptr == NULL)\
     {\
        fprintf(stderr, "error: out of memory\n");\
        returncode = EXIT_FAILURE;\
        cleanup(0);\
     }\
     __ptr;\
     })

#endif
