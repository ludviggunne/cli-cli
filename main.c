
#include <sysexits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <signal.h>

#include "async_read.h"
#include "tui.h"

#define _assert(x) { int __line = __LINE__;\
    if (!(x)) {\
        fprintf(stderr, "error: %d: %s: %s\n",\
                __line, #x, strerror(errno));\
        returncode = EXIT_FAILURE;\
        cleanup(0);\
    }}

union pipe {
    struct { int r, w; };
    int arr[2];
};

int returncode = EXIT_SUCCESS;
int pid = -1;
const char * chprocname = NULL;
char ** chargs = NULL;
union pipe pipe_p2c = { .r = -1, .w = -1, };
union pipe pipe_c2p = { .r = -1, .w = -1, };
struct async_read child_rd;

void cleanup(int sig)
{
    (void) sig;
    tui_terminate();
    async_read_stop(&child_rd);
    if (pipe_p2c.r != -1) close(pipe_p2c.r);
    if (pipe_p2c.w != -1) close(pipe_p2c.w);
    if (pipe_c2p.r != -1) close(pipe_c2p.r);
    if (pipe_c2p.w != -1) close(pipe_c2p.w);
    if (chargs) free(chargs);
    exit(returncode);
}

void usage(const char * name)
{
    fprintf(stdout, "usage: %s PROGRAM [ARGS...]\n", name);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "error: missing input\n");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    signal(SIGINT, cleanup);

    chprocname = argv[1];

    chargs = malloc(sizeof (*chargs) * argc);
    memcpy(chargs, &argv[1], sizeof (*chargs) * (argc - 1));
    chargs[argc - 1] = NULL;

    if (pipe(pipe_p2c.arr) != 0 || pipe(pipe_c2p.arr) != 0)
    {
        fprintf(stderr, "error: unable to open pipe(s): %s\n", strerror(errno));
        returncode = EXIT_FAILURE;
        cleanup(0);
    }

    pid = fork();
    if (pid < 0)
    {
        fprintf(stderr, "error: unable to fork: %s\n", strerror(errno));
        returncode = EXIT_FAILURE;
        cleanup(0);
    }
    else if (pid == 0)
    {
        _assert(close(pipe_p2c.w) > -1);
        _assert(close(pipe_c2p.r) > -1);
        _assert(dup2(pipe_p2c.r, STDIN_FILENO) > -1);
        _assert(dup2(pipe_c2p.w, STDOUT_FILENO) > -1);
        _assert(close(pipe_p2c.r) > -1);
        _assert(close(pipe_c2p.w) > -1);

        if (execvp(chprocname, chargs) == -1)
        {
            fprintf(stderr, "error: unable to start process %s: %s\n", chprocname, strerror(errno));
            returncode = EXIT_FAILURE;
            cleanup(0);
        }
    }
    else
    {
        FILE * child_r_fp, * child_w_fp;
        char * line;

        _assert(close(pipe_p2c.r) > -1);
        _assert(close(pipe_c2p.w) > -1);

        child_r_fp = fdopen(pipe_c2p.r, "r");
        child_w_fp = fdopen(pipe_p2c.w, "w");

        _assert(child_r_fp != NULL);
        _assert(child_w_fp != NULL);

        child_rd = async_read_create(child_r_fp);
        async_read_start(&child_rd);

        tui_init();

        while (waitpid(pid, &returncode, WNOHANG) == 0)
        {
            line = async_read_line(&child_rd);
            if (line)
            {
                tui_write_line(line);
                free(line);
            }

            line = tui_read_line();
            if (line)
            {
                fprintf(child_w_fp, "%s\n", line);
                fflush(child_w_fp);
                free(line);
            }

            tui_update();

            usleep(10);
        }

        while ((line = async_read_line(&child_rd)))
        {
            tui_write_line(line);
        }

        tui_prompt_exit(returncode);
    }

    cleanup(0);
}
