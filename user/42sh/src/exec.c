/*
** exec.c for exec in /home/rebut_p/Programmation/Projet-b2/sys_unix/42sh/src
**
** Made by Pierre REBUT
** Login   <rebut_p@epitech.net>
**
** Started on  Fri May 15 13:59:03 2015 Pierre REBUT
** Last update Sun Jun  7 15:25:50 2015 dourch_m
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "functions.h"
#include "define.h"

static int do_pid_zero(t_cmd *lst, t_env *env)
{
    struct ExceveInfo execInfo = INIT_EXECINFO();
    execInfo.av = (const char **) lst->args;
    execInfo.env = (const char **) env->env;

    if (lst->prev != NULL) {
        execInfo.fd_in = lst->pipefd[0];
    } else if (lst->redir_in != NULL) {
        execInfo.fd_in = exe_redir_in(lst);
        if (execInfo.fd_in == -1)
            return (ERROR_FILS);
    }

    if (lst->next != NULL) {
        execInfo.fd_out = lst->next->pipefd[1];
    } else if (lst->redir_out != NULL) {
        execInfo.fd_out = exe_redir_out(lst, -1);
        if (execInfo.fd_out == -1)
            return (ERROR_FILS);
    }

    int ret = exec_prg(&execInfo, lst, env);
    if (execInfo.fd_out != -1)
        close(execInfo.fd_out);
    if (execInfo.fd_in != -1)
        close(execInfo.fd_in);

    return ret;
}

static int do_father(t_cmd *tmp)
{
    int ret = 0;

    while (tmp->prev != NULL) {
        close(tmp->pipefd[0]);
        close(tmp->pipefd[1]);
        tmp = tmp->prev;
    }
    waitpid(-1);
    return (ret);
}


static int exec_do_first(t_cmd *lst, t_env *env, t_cmd *tmp)
{
    while (lst != NULL) {
        if (lst->prev != NULL)
            if (pipe(lst->pipefd) == -1)
                return (printf("Error : pipe\n"));
        if (exec_built(lst, env) == FAIL) {
            if (do_pid_zero(lst, env) == ERROR_FILS)
                return (0);
        }
        lst = lst->prev;
    }
    return (do_father(tmp));
}

int exec_lst(t_cmd *lst, t_env *env)
{
    return exec_do_first(lst, env, lst);
}
