
#include <curses.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "tui.h"
#include "mem.h"

#define CMD_BUFFER_MAX_SIZE 64
#define CMD_WINDOW_HEIGHT 3
#define CMD_WINDOW_Y (LINES - CMD_WINDOW_HEIGHT)
#define CMD_BUFFER_X 5
#define CMD_BUFFER_Y 1
#define CHILD_WINDOW_HEIGHT CMD_WINDOW_Y
#define CHILD_WINDOW_FIRST_LINE 2
#define CHILD_WINDOW_LAST_LINE (CHILD_WINDOW_HEIGHT - 2)
#define TITLE_X 3

#define COLOR_MATCH COLOR_BLUE // we change this to gray
#define COLOR_ERROR COLOR_RED
#define COLOR_OK COLOR_GREEN

enum color_pair {
    COLOR_PAIR_MATCH = 1,
    COLOR_PAIR_ERROR,
    COLOR_PAIR_OK,
};

extern char ** chargs;
extern int returncode;
extern int pid;
extern void cleanup(int);

static char cmdbuf[CMD_BUFFER_MAX_SIZE + 1];
static int cmdbufsz = 0;
static int matchid = -1;
static WINDOW * cmd_win = NULL;
static WINDOW * child_win = NULL;
static char * ready_line = NULL;

static char ** history = NULL;
static int hcap = 128;
static int hcount = 0;

static void draw_cmdbuf(void)
{
    mvwaddnstr(cmd_win, CMD_BUFFER_Y, CMD_BUFFER_X, cmdbuf, cmdbufsz);
}

static void clear_cmdbuf(void)
{
    cmdbufsz = 0;
}

static void clear_cmdbuf_line(void)
{
    wmove(cmd_win, CMD_BUFFER_Y, CMD_BUFFER_X);
    wclrtoeol(cmd_win);
}

static void draw_match(void)
{
    if (matchid > -1)
    {
        wattron(cmd_win, COLOR_PAIR(COLOR_PAIR_MATCH));
        mvwaddstr(cmd_win, CMD_BUFFER_Y, CMD_BUFFER_X, history[matchid]);
        wattroff(cmd_win, COLOR_PAIR(COLOR_PAIR_MATCH));
    }
}

static int try_match(int id)
{
    const char * str = history[id];
    return memcmp(str, cmdbuf, cmdbufsz) == 0;
}

static void match_reset(void)
{
    matchid = -1;
    for (int i = hcount - 1; i >= 0; i--)
    {
        if (try_match(i))
        {
            matchid = i;
            return;
        }
    }
}

static void complete_match(void)
{
    if (matchid > -1)
    {
        const char * cmd = history[matchid];
        strcpy(cmdbuf, cmd);
        cmdbufsz = strlen(cmd);
    }
}

static void complete_match_word(void)
{
    if (matchid > -1)
    {
        int i = cmdbufsz;
        const char * cmd = history[matchid];
        while (cmd[i] != ' ' && cmd[i] != '\0')
        {
            i++;
        }
        while (cmd[i] == ' ')
        {
            i++;
        }
        strncpy(cmdbuf, cmd, i);
        cmdbufsz = i;
    }
}

static void history_append(const char * str, int len)
{
    if (hcount == hcap)
    {
        hcap *= 2;
        char ** nh = malloc(sizeof (*nh) * hcap);
        memcpy(nh, history, sizeof (*nh) * hcount);
        free(history);
        history = nh;
    }

    char * append = strndup(str, (size_t) len);
    history[hcount++] = append;
}

static void match_back(void)
{
    if (matchid > -1)
    {
        for (int i = matchid - 1; i >= 0; i--)
        {
            if (try_match(i))
            {
                matchid = i;
                return;
            }
        }
    }
    else
    {
        match_reset();
    }
}

static void match_forward(void)
{
    if (matchid > -1)
    {
        for (int i = matchid + 1; i < hcount; i++)
        {
            if (try_match(i))
            {
                matchid = i;
                return;
            }
        }
    }
}

static void scroll_output(void)
{
    wmove(child_win, CHILD_WINDOW_FIRST_LINE, 0);
    wdeleteln(child_win);
    wmove(child_win, CHILD_WINDOW_LAST_LINE, 0);
    wclrtoeol(child_win);
}

static void child_refresh(void)
{
    box(child_win, 0, 0);
    wmove(child_win, 0, TITLE_X);
    char ** arg = chargs;
    waddch(child_win, ' ');
    while (*arg)
    {
        wprintw(child_win, "%s ", *arg);
        arg++;
    }
    wprintw(child_win, "(pid: %d) ", pid);
    wrefresh(child_win);
}

static void cmd_refresh(void)
{
    clear_cmdbuf_line();
    draw_match();
    draw_cmdbuf();
    box(cmd_win, 0, 0);
    mvwaddstr(cmd_win, 0, TITLE_X, " Input ");
    mvwaddstr(cmd_win, 1, 2, ">>");
    wrefresh(cmd_win);
}

static void flush_cmdbuf(void)
{
    if (ready_line)
    {
        free(ready_line);
    }
    ready_line = strndup(cmdbuf, cmdbufsz);

    int is_previous;
    if (hcount > 0)
    {
        const char * previous = history[hcount - 1];
        if ((int) strlen(previous) == cmdbufsz &&
                strncmp(previous, cmdbuf, cmdbufsz) == 0)
        {
            is_previous = 1;
        }
        else
        {
            is_previous = 0;
        }
    }
    else
    {
        is_previous = 0;
    }

    if (cmdbufsz > 0 && !is_previous)
    {
        history_append(cmdbuf, cmdbufsz);
    }
    matchid = -1;
    //match_reset();
}

static void cmdbuf_write_char(int ch)
{
    if (cmdbufsz < CMD_BUFFER_MAX_SIZE)
    {
        cmdbuf[cmdbufsz++] = ch;
    }
}

static void cmdbuf_del_char(void)
{
    if (cmdbufsz > 0)
    {
        cmdbufsz--;
    }
}

void tui_init(void)
{
    initscr();

    if (!has_colors() || !can_change_color())
    {
        fprintf(stderr, "error: terminal doesn't have required color capabilities\n");
        returncode = EXIT_FAILURE;
        cleanup(0);
    }

    start_color();
    use_default_colors();
    init_color(COLOR_MATCH,  500, 500, 500);
    init_color(COLOR_ERROR, 800, 200, 200);
    init_pair(COLOR_PAIR_MATCH, COLOR_MATCH, -1);
    init_pair(COLOR_PAIR_ERROR, COLOR_ERROR, -1);
    init_pair(COLOR_PAIR_OK, COLOR_OK, -1);

    cmd_win = newwin(CMD_WINDOW_HEIGHT, 0, CMD_WINDOW_Y, 0);
    child_win = newwin(CHILD_WINDOW_HEIGHT, 0, 0, 0);

    noecho();
    curs_set(0);
    keypad(cmd_win, TRUE);
    nodelay(cmd_win, TRUE);

    history = malloc(sizeof (*history) * hcap);

    child_refresh();
    cmd_refresh();
}

void tui_write_line(const char * line)
{
    scroll_output();
    mvwaddstr(child_win, CHILD_WINDOW_LAST_LINE, 2, line);
    //cmd_refresh();
    child_refresh();
}

void tui_write_error(const char * line)
{
    scroll_output();
    wattron(child_win, COLOR_PAIR(COLOR_PAIR_ERROR));
    mvwaddstr(child_win, CHILD_WINDOW_LAST_LINE, 2, line);
    wattroff(child_win, COLOR_PAIR(COLOR_PAIR_ERROR));
    //cmd_refresh();
    child_refresh();
}

char * tui_read_line(void)
{
    char * line;
    line = ready_line;
    ready_line = NULL;
    return line;
}

void tui_update(void)
{
    int update = 0;

    int in = wgetch(cmd_win);

    switch (in)
    {
        case KEY_BACKSPACE:
        {
            cmdbuf_del_char();
            if (matchid == -1)
            {
                match_reset();
            }
            update = 1;
            break;
        }
        case KEY_LEFT:
        {
            scroll_output();
            update = 1;
            break;
        }
        case '\t':
        case KEY_RIGHT:
        {
            //complete_match();
            complete_match_word();
            match_reset();
            update = 1;
            break;
        }
        case KEY_UP:
        {
            match_back();
            update = 1;
            break;
        }
        case KEY_DOWN:
        {
            match_forward();
            update = 1;
            break;
        }
        case '\n':
        {
            flush_cmdbuf();
            clear_cmdbuf();
            update = 1;
            break;
        }
        case ERR:
        {
            // no input
            update = 0;
            break;
        }
        default:
        {
            if (isprint(in))
            {
                cmdbuf_write_char(in);
                if (matchid == -1 || !try_match(matchid))
                {
                    match_reset();
                }
                update = 1;
            }
            else
            {
                update = 0;
            }
            break;
        }
    }

    if (update)
    {
        cmd_refresh();
        child_refresh();
    }
}

void tui_terminate(void)
{
    delwin(cmd_win);
    delwin(child_win);
    endwin();
    if (history) {
        for (int i = 0; i < hcount; i++)
        {
            free(history[i]);
        }
        free(history);
    }
}

void tui_prompt_exit(int code)
{
    nodelay(cmd_win, FALSE);
    clear_cmdbuf_line();
    int pair = code == 0 ? COLOR_PAIR_OK : COLOR_PAIR_ERROR;
    wattron(cmd_win, COLOR_PAIR(pair));
    mvwprintw(cmd_win, CMD_BUFFER_Y, CMD_BUFFER_X, "%s exited with return code %d. Press any key to exit.",
            chargs[0], code);
    wattroff(cmd_win, COLOR_PAIR(pair));

    box(cmd_win, 0, 0);
    wrefresh(cmd_win);
    child_refresh();
    (void) wgetch(cmd_win);
}
