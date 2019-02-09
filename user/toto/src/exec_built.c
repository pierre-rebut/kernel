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
#include <syscallw.h>
#include <string.h>

#include "struct.h"
#include "define.h"
#include "functions.h"

int redir_out_fd(t_cmd *lst) {
    int fd;
    
    if (lst->redir_out[0] == '>')
        fd = open(lst->redir_out + 1, O_APPEND | O_RDWR | O_CREAT);
    else
        fd = open(lst->redir_out, O_WRONLY | O_CREAT);
    return (fd);
}

int exec_built(t_cmd *lst, t_env *env) {
    int fd;

    fd = 1;
    if (lst->next != NULL)
        fd = lst->next->pipefd[1];
    else if (lst->redir_out != NULL)
        fd = redir_out_fd(lst);
    if (strcmp("cd", lst->prg) == SUCCESS)
        cd_call(lst->args[1], env, fd);
    else if (strcmp("env", lst->prg) == SUCCESS)
        env_call(env, fd);
    else if (strcmp("setenv", lst->prg) == SUCCESS)
        setenv_call(lst->args[1], lst->args[2], env);
    else if (strcmp("unsetenv", lst->prg) == SUCCESS)
        unsetenv_call(lst->args[1], env);
    else if (strcmp("echo", lst->prg) == SUCCESS)
        echo(lst, fd);
    else
        return (FAIL);
    if (fd > 2)
        close(fd);
    return (SUCCESS);
}
