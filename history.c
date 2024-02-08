
#include <stdlib.h>
#include <string.h>

#include "history.h"

struct cmdn {
    char * cmd;
    struct cmdn * next;
    struct cmdn * prev;
};

struct cmdn * cmdn_append_n(struct cmdn * head, const char * cmd, int len)
{
    struct cmdn * cmdn;

    cmdn = malloc(sizeof (*cmdn));

    if (head == NULL)
    {
        cmdn->next = NULL;
    }
    else
    {
        cmdn->next = head;
        head->prev = cmdn;
    }

    cmdn->prev = NULL;
    cmdn->cmd = malloc(sizeof (*cmdn->cmd) * (len + 1));
    memcpy(cmdn->cmd, cmd, len);
    cmdn->cmd[len] = '\0';

    return cmdn;
}

struct cmdn * cmdn_append(struct cmdn * head, const char * cmd)
{
    return cmdn_append_n(head, cmd, strlen(cmd));
}

void cmdn_destroy(struct cmdn * head)
{
    struct cmdn * tmp;

    while (head)
    {
        free(head->cmd);
        tmp = head->next;
        free(head);
        head = tmp;
    }
}

struct cmdn * cmdn_next(struct cmdn * cmdn)
{
    if (cmdn != NULL)
    {
        return cmdn->next;
    }
    else
    {
        return NULL;
    }
}

struct cmdn * cmdn_prev(struct cmdn * cmdn)
{
    if (cmdn != NULL)
    {
        return cmdn->prev;
    }
    else
    {
        return NULL;
    }
}

const char * cmdn_get_cmd(struct cmdn * cmdn)
{
    if (cmdn)
    {
        return cmdn->cmd;
    }
    else
    {
        return NULL;
    }
}

int cmdn_history_len(struct cmdn * head)
{
    int len = 0;
    while (head)
    {
        len++;
        head = head->next;
    }
    return len;
}
