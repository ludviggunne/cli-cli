
#include <curses.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "tui.h"
#include "history.h"

#define IN_WIN_H 3
#define IN_WIN_Y (LINES - IN_WIN_H)
#define OUT_WIN_H (LINES - IN_WIN_H)
#define OUT_WIN_FIRSTLN 1
#define OUT_WIN_LASTLN (OUT_WIN_H - 2)
#define OUT_LINE_X 1
#define IN_FIELD_X 4
#define INPUTBUF_CAP 64
#define TITLE_X 3

enum input_state {
    INPUT,
    HISTORY,
};

static WINDOW *in_win = NULL, *out_win = NULL;
static char inputbuf[INPUTBUF_CAP + 1];
static char * input_line = NULL;
static int inputbuf_size = 0;
static int tui_is_init = 0;
static const char * _child_name;
static int state = INPUT;
static struct cmdn * head = NULL;
static struct cmdn * curr_cmdn = NULL;

static void handle_char_input(int ch);
static void handle_char_history(int ch, int first);

void tui_init(const char * child_name)
{
    initscr();

    _child_name = child_name;

    noecho();
    curs_set(0);

    in_win = newwin(IN_WIN_H, 0, IN_WIN_Y, 0);
    out_win = newwin(OUT_WIN_H, 0, 0, 0);
    keypad(in_win, TRUE);

    nodelay(in_win, TRUE);

    tui_is_init = 1;
}

void tui_write_output_line(const char * line)
{
    wmove(out_win, OUT_WIN_FIRSTLN, 0);
    wdeleteln(out_win);
    wmove(out_win, OUT_WIN_LASTLN, 0);
    wdeleteln(out_win);
    mvwprintw(out_win, OUT_WIN_LASTLN, OUT_LINE_X, "%.*s", (int)(strlen(line) - 1), line);
}

void tui_write_output_line_n(const char * line, int length)
{
    (void) line; (void) length;
}

char * tui_get_input_line(void)
{
    char * line;
    line = input_line;
    input_line = NULL;
    return line;
}

static void flush_input()
{
    inputbuf[inputbuf_size] = '\0';

    // TODO: rework. we lose a line if main hasn't read the previous line
    if (input_line)
    {
        free(input_line);
    }
    input_line = strdup(inputbuf);
    wmove(in_win, 1, 0);
    wclrtoeol(in_win);
    if (state != HISTORY)
    {
        head = cmdn_append_n(head, inputbuf, inputbuf_size);
    }
    inputbuf_size = 0;
    state = INPUT;
}

static void handle_char_input(int ch)
{
    switch (ch)
    {
        case KEY_BACKSPACE:
        {
            if (inputbuf_size > 0)
            {
                inputbuf_size--;
                mvwdelch(in_win, 1, IN_FIELD_X + inputbuf_size);
                mvwdelch(in_win, 1, COLS - 2);
            }
            break;
        }
        case KEY_UP:
        {
            state = HISTORY;
            handle_char_history(ch, 1);
            break;
        }
        case 0xa:
        {
            flush_input();
            return;
        }
        case ERR:
            break;
        default:
        {
            if (isprint(ch) && inputbuf_size < INPUTBUF_CAP) {
                mvwaddch(in_win, 1, IN_FIELD_X + inputbuf_size, ch);
                inputbuf[inputbuf_size] = ch;
                inputbuf_size++;
            }
            break;
        }
    }
}

static void handle_char_history(int ch, int first)
{
    struct cmdn * tmp;
    const char * cmd = NULL;

    if (head == NULL)
    {
        state = INPUT;
        return;
    }

    switch (ch)
    {
        case KEY_UP:
        {
            if (!first)
            {
                tmp = cmdn_next(curr_cmdn);
                if (tmp)
                {
                    curr_cmdn = tmp;
                }
            }
            else
            {
                curr_cmdn = head;
            }
            cmd = cmdn_get_cmd(curr_cmdn);
            break;
        }
        case KEY_DOWN:
        {
            tmp = cmdn_prev(curr_cmdn);
            if (tmp)
            {
                curr_cmdn = tmp;
            }
            cmd = cmdn_get_cmd(curr_cmdn);
            break;
        }
        case 0xa:
        {
            flush_input();
            state = INPUT;
            return;
        }
        default:
        {
            state = INPUT;
            handle_char_input(ch);
            return;
        }
    }


    inputbuf_size = strlen(cmd);
    strcpy(inputbuf, cmd);

    wmove(in_win, 1, 0);
    wclrtoeol(in_win);
    mvwprintw(in_win, 1, IN_FIELD_X, cmd);
}

void tui_update(void)
{
    int ch;

    ch = wgetch(in_win);

    if (ch != ERR)
    {
        switch (state)
        {
            case HISTORY:
            {
                handle_char_history(ch, 0);
                break;
            }
            case INPUT:
            {
                handle_char_input(ch);
                break;
            }
        }
    }

    mvwaddstr(in_win, 1, 1, ">>");

    box(in_win, 0, 0);
    box(out_win, 0, 0);

    mvwprintw(out_win, 0, TITLE_X, " Process: %s ", _child_name);
    const char * statestr = "";
    switch (state) {
        case HISTORY: statestr = "History"; break;
        case INPUT: statestr = "Input"; break;
    }
    mvwprintw(in_win, 0, TITLE_X, " %s ", statestr);

    wrefresh(in_win);
    wrefresh(out_win);
}

void tui_terminate(void)
{
    if (tui_is_init)
    {
        delwin(in_win);
        delwin(out_win);
        endwin();
        cmdn_destroy(head);
    }
}

void tui_wait_for_exit(int code)
{
    const char * prompt_fmt = "%s exited with return code %d. Press any key to exit...";
    assert(strlen(prompt_fmt) + 1 <= INPUTBUF_CAP);
    wmove(in_win, 1, 1);
    wclrtoeol(in_win);
    mvwprintw(in_win, 1, 1, prompt_fmt, _child_name, code);
    box(in_win, 0, 0);
    nodelay(in_win, FALSE);
    (void) wgetch(in_win);
}
