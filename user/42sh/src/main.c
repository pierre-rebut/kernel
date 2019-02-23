/*
** main.c for 42sh in /home/despla_s/Epitech/Systeme_Unix/PSU_2014_42sh/src
**
** Made by Pierre REBUT
** Login   <rebut_p@epitech.net>
**
** Started on  Wed May  6 14:37:40 2015 Pierre REBUT
** Last update Sun May 24 17:37:04 2015 Pierre REBUT
*/

#include <stdlib.h>
#include <stdio.h>
#include <filestream.h>

#include "define.h"
#include "functions.h"

int parse_tab(char **tab, const int *i, t_env *env, int *exit_val)
{
    int ret;

    ret = 0;
    if ((tab[*i] = epur_str(tab[*i])) == NULL)
        return (-1);
    if (parser(tab[*i]) == SUCCESS) {
        if ((ret = exec_line(tab[*i], env, exit_val)) == ERROR_FILS)
            return (ERROR_FILS);
    } else
        ret = 1;
    return (ret);
}

int parse_cmd(char *cmd, t_env *env, int ret, int *exit_val)
{
    int i;
    char **tab;

    i = -1;
    if ((tab = str_parser_line(cmd, 0, 0)) != NULL) {
        while (tab[++i]) {
            if (check_tab(tab, i++, ret) == 0) {
                ret = parse_tab(tab, &i, env, exit_val);
                if (ret == ERROR_FILS)
                    return (ERROR_FILS);
                else if (ret == -1)
                    return (-1);
            }
        }
        free_tab(tab);
    } else
        printf("Invalid null command.\n");
    free(cmd);
    return (0);
}

/*
void get_sigint(int sig) {
    if (sig == SIGINT) {
        if (g_pid != 0)
            kill(g_pid, SIGINT);
    }
}

int get_signal(void) {
    if ((signal(SIGINT, &get_sigint)) == SIG_ERR)
        return (FAIL);
    if ((signal(SIGQUIT, &get_sigint)) == SIG_ERR)
        return (FAIL);
    if ((signal(SIGTSTP, &get_sigint)) == SIG_ERR)
        return (FAIL);
    return (SUCCESS);
}*/

int main(int ac, char **av, char **env)
{
    int exit_val;
    t_env tenv;
    char *cmd;

    (void) ac;
    (void) av;
    exit_val = 0;
    if ((tenv.env = create_env(env)) == NULL)
        return (FAIL);
    //if (get_signal() == FAIL)
    //    return (FAIL);
    while (42) {
        puts("$> ");
        if ((cmd = getline(stdin)) == NULL) {
            free_tab(tenv.env);
            puts("exit\n");
            return (exit_val);
        }
        if (cmd[0] != 0 && (cmd = my_epur_cmd(cmd, 0)) != NULL)
            if (parse_cmd(cmd, &tenv, 0, &exit_val) == ERROR_FILS)
                return (exit_val);
    }
}
