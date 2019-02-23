/*
** exec_built.c for 42sh in /home/despla_s/Epitech/Systeme_Unix/PSU_2014_42sh/src
**
** Made by despla_s
** Login   <despla_s@epitech.net>
**
** Started on  Fri May 22 14:11:13 2015 despla_s
** Last update Sun Jun  7 15:25:49 2015 dourch_m
*/

#include <kstd.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>

#include "struct.h"
#include "define.h"
#include "functions.h"

struct BuiltinCmd
{
    char *name;

    int (*fct)(int fd, t_cmd *lst, t_env *env);
};

static struct BuiltinCmd lstCmd[] = {
        {"cd",       cd_call},
        {"env",      env_call},
        {"setenv",   setenv_call},
        {"unsetenv", unsetenv_call},
        {"echo",     echo},
        {"sync",     sync_call},
        {"kill",     kill_call},
        {NULL}
};

int redir_out_fd(t_cmd *lst)
{
    int fd;

    if (lst->redir_out[0] == '>')
        fd = open(lst->redir_out + 1, O_WRONLY | O_CREAT | O_APPEND, 0x180);
    else
        fd = open(lst->redir_out, O_WRONLY | O_CREAT | O_TRUNC, 0x180);
    return (fd);
}

int exec_built(t_cmd *lst, t_env *env)
{
    int i = 0;

    while (lstCmd[i].name) {
        if (strcmp(lstCmd[i].name, lst->prg) == 0) {
            int fd = 1;
            if (lst->next != NULL)
                fd = lst->next->pipefd[1];
            else if (lst->redir_out != NULL)
                fd = redir_out_fd(lst);

            if (fd < 0) {
                printf("42sh : %s\n", strerror(errno));
                return FAIL;
            }

            int res = lstCmd[i].fct(fd, lst, env);
            if (fd > 2)
                close(fd);
            return res;
        }
        i++;
    }

    return FAIL;
}
