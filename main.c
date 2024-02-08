
#include <sysexits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>

#include "async_reader.h"

#define try(x) {int __line = __LINE__;\
    errno = 0;\
    x;\
    if (errno != 0)\
    {\
        fprintf(stderr, "error on line %d: %s: %s\n", __line, #x, strerror(errno));\
        retcode = EXIT_FAILURE;\
        goto cleanup;\
    }}

void usage(const char * name)
{
    printf("USAGE:\n"
           "    %s [program]\n", name);
}

int main(int argc, char *argv[])
{
    FILE * child_read_fp = NULL;
    FILE * child_write_fp = NULL;
    int pipe_1[2] = { -1, -1, };
    int pipe_2[2] = { -1, -1, };
    int child_stdout, child_stdin;
    int child_read_fd, child_write_fd;
    struct reader child_reader, user_reader;
    char * child_name = NULL;
    int retcode;
    pid_t pid;

    retcode = EXIT_SUCCESS;

    if (argc < 2)
    {
        fprintf(stderr, "error: no input\n");
        usage(argv[0]);
        return EX_USAGE;
    }
    child_name = argv[1];

    try( pipe(pipe_1) );
    try( pipe(pipe_2) );

    child_stdin  = pipe_1[0];
    child_stdout = pipe_2[1];
    child_write_fd = pipe_1[1];
    child_read_fd   = pipe_2[0];

    pid = fork();
    if (pid < 0)
    {
        fprintf(stderr, "error: unable to fork process: %s\n", strerror(errno));
        return EX_OSERR;
    }
    else if (pid == 0)
    {
        try( close(child_read_fd) );
        try( close(child_write_fd) );
        try( dup2(child_stdin, STDIN_FILENO) );
        try( dup2(child_stdout, STDOUT_FILENO) );
        try( close(child_stdin) );
        try( close(child_stdout) );

        char * * args = malloc(sizeof (*args) * argc);
        memcpy(args, &argv[1], sizeof (*args) * (argc - 1));
        args[argc - 1] = NULL;
        try( execvp(child_name, args) );
    }
    else
    {
        try( child_read_fp = fdopen(child_read_fd, "r") );
        try( child_write_fp = fdopen(child_write_fd, "w") );

        child_reader = reader_create(child_read_fp);
        user_reader = reader_create(stdin);

        reader_start(&child_reader);
        reader_start(&user_reader);

        while (waitpid(pid, &retcode, WNOHANG) == 0)
        {
            char * line;

            if ((line = reader_getline(&user_reader)) != NULL)
            {
                fprintf(child_write_fp, "%s", line);
                fflush(child_write_fp);
                free(line);
            }

            if ((line = reader_getline(&child_reader)) != NULL)
            {
                fprintf(stdout, "%s: %s", child_name, line);
                free(line);
            }

            usleep(10);
        }

        goto cleanup;
    }

cleanup:
    reader_stop(&child_reader);
    reader_stop(&user_reader);
    if (pipe_1[0] != -1) close(pipe_1[0]);
    if (pipe_1[1] != -1) close(pipe_1[1]);
    if (pipe_2[0] != -1) close(pipe_2[0]);
    if (pipe_2[1] != -1) close(pipe_2[1]);

    return retcode;
}
