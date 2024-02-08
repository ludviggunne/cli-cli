
#include "tui.h"
#include <curses.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define IN_WIN_H 3
#define IN_WIN_Y (LINES - IN_WIN_H)
#define OUT_WIN_H (LINES - IN_WIN_H)
#define OUT_WIN_FIRSTLN 1
#define OUT_WIN_LASTLN (OUT_WIN_H - 2)
#define OUT_LINE_X 1
#define IN_FIELD_X 4
#define INPUTBUF_CAP 64

static WINDOW *in_win = NULL, *out_win = NULL;
static char inputbuf[INPUTBUF_CAP + 1];
static char * input_line = NULL;
static int inputbuf_size = 0;
static int tui_is_init = 0;
static const char * _child_name;

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
    //mvwaddnstr(out_win, OUT_WIN_LASTLN, OUT_LINE_X, line, strlen(line) - 1);
    mvwprintw(out_win, OUT_WIN_LASTLN, OUT_LINE_X, "%s: %.*s", _child_name, (int)(strlen(line) - 1), line);
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
    wmove(in_win, IN_WIN_Y + 1, 0);
    wclrtoeol(in_win);
    inputbuf_size = 0;
}

void tui_update(void)
{
    int ch;

    mvwaddstr(in_win, 1, 1, ">>");

    ch = wgetch(in_win);

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
        case 0xa:
        {
            flush_input();
            break;
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

    box(in_win, 0, 0);
    box(out_win, 0, 0);
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
