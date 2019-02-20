/*
** parser.c for 42sh in /home/rebut_p/Programmation/Projet-b2/sys_unix/42sh
**
** Made by Pierre REBUT
** Login   <rebut_p@epitech.net>
**
** Started on  Sun May 24 17:34:29 2015 Pierre REBUT
** Last update Sat Jun  6 17:16:12 2015 dourch_m
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <alloc.h>

#include "define.h"
#include "functions.h"

int parser(char *cmd)
{
    int ret;
    char *tmp;

    tmp = strdup(cmd);
    if (tmp == NULL || tmp[0] == '\0')
        return (FAIL);
    ret = check_expr_cmplx(epur_str(tmp), 0);
    if (tmp != NULL)
        free(tmp);
    if (ret == FAIL)
        return (FAIL);
    else
        return (SUCCESS);
}

int check_ex(char *str, int *exit_val)
{
    if (check_nb(str) == 0)
        *exit_val = atoi(str);
    else {
        printf("exit: Expression Syntax.\n");
        return (1);
    }
    return (0);
}

int exec_line(char *cmd, t_env *env, int *exit_val)
{
    int ret;
    char **tab;
    t_cmd *lst;

    tab = my_wordtab(cmd, ' ', 0, 0);
    if (tab == NULL)
        return (FAIL);
    if (strcmp(tab[0], "exit") == 0) {
        if (tab[1] != NULL)
            if (check_ex(tab[1], exit_val) == 1)
                return (-2);
        return (ERROR_FILS);
    }
    lst = create_list(tab);
    free_tab(tab);
    if (lst == NULL)
        return (1);
    if (check_pipe(lst) == -1)
        return (1);
    ret = exec_lst(lst, env);
    free_list(lst);
    return (ret);
}

int check_tab(char **tab, int i, int value)
{
    if (tab[i + 1] != NULL) {
        if (strcmp(tab[i + 1], ";") == 0) {
            return (1);
        }
        if (strcmp(tab[i], "&&") == 0 && value != 0)
            return (1);
        else if (strcmp(tab[i], "||") == 0 && value == 0)
            return (1);
        else
            return (0);
    }
    return (1);
}
