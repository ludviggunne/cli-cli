
#ifndef TUI_H
#define TUI_H

#include <stddef.h>

void tui_init(const char * child_name);
void tui_write_output_line(const char * line);
void tui_write_output_line_n(const char * line, int length);
char * tui_get_input_line(void);
void tui_update(void);
void tui_terminate(void);
void tui_wait_for_exit(int code);

#endif
