
#ifndef HISTORY_H
#define HISTORY_H

struct cmdn;

struct cmdn * cmdn_append_n(struct cmdn * head, const char * cmd, int len);
struct cmdn * cmdn_append(struct cmdn * head, const char * cmd);
struct cmdn * cmdn_next(struct cmdn * cmdn);
struct cmdn * cmdn_prev(struct cmdn * cmdn);
void cmdn_destroy(struct cmdn * head);
const char * cmdn_get_cmd(struct cmdn * cmdn);
int cmdn_history_len(struct cmdn * head);

#endif
