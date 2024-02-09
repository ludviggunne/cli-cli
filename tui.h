
#ifndef TUI_H
#define TUI_H

#include <stddef.h>

void tui_init(void);
void tui_write_line(const char * line);
void tui_write_error(const char * line);
char * tui_read_line(void);
void tui_update(void);
void tui_terminate(void);
void tui_prompt_exit(int code);

#endif
