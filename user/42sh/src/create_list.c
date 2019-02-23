/*
** create_list.c for list in /home/martin/rendu/Projet/PSU_2014_42sh/src
**
** Made by dourch_m
** Login   <martin@epitech.net>
**
** Started on  Wed May 13 16:13:46 2015 dourch_m
** Last update Sun May 24 17:55:09 2015 Pierre REBUT
*/

#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include "struct.h"
#include "functions.h"

int my_select_list(t_cmd *elem, char *cmd, int i)
{
    if (elem->prg == NULL) {
        elem->args = malloc(sizeof(char *) * 3);
        if (elem->args == NULL)
            return (-1);
        if ((elem->args[i++] = strdup(cmd)) == NULL)
            return (-1);
        elem->args[i] = NULL;
        if ((elem->prg = strdup(cmd)) == NULL)
            return (-1);
    } else {
        elem->args = realloc(elem->args, sizeof(char *) * (tab_len(elem->args) + 3));
        if (elem == NULL)
            return (-1);
        while (elem->args[i] != NULL)
            i++;
        if ((elem->args[i++] = strdup(cmd)) == NULL)
            return (-1);
        elem->args[i] = NULL;
    }
    return (0);
}

int get_redir(t_cmd *lst, char *cmd, int op)
{
    if (op == 5) {
        if ((lst->redir_in = strcat("<", cmd)) == NULL)
            return (-1);
    } else if (op == 6) {
        if ((lst->redir_out = strcat(">", cmd)) == NULL)
            return (-1);
    } else {
        if ((my_select_list(lst, cmd, 0)) == -1)
            return (-1);
    }
    return (0);
}

t_cmd *my_put_in_list(t_cmd *lst, char *cmd, int op)
{
    t_cmd *elem;

    if (op == -1) {
        elem = init_list();
        if (elem == NULL)
            return (NULL);
        elem->prev = lst;
        lst->next = elem;
        lst = elem;
    } else if (op == 3) {
        if ((lst->redir_in = strdup(cmd)) == NULL)
            return (NULL);
    } else if (op == 4) {
        if ((lst->redir_out = strdup(cmd)) == NULL)
            return (NULL);
    } else if (get_redir(lst, cmd, op) == -1)
        return (NULL);
    return (lst);
}

t_cmd *do_check_param(t_cmd *lst, char **cmd, int *i)
{
    if (strcmp(cmd[*i], ">>") == 0) {
        if ((lst = my_put_in_list(lst, cmd[++(*i)], 6)) == NULL)
            return (NULL);
    } else if (strcmp(cmd[*i], "<<") == 0) {
        if ((lst = my_put_in_list(lst, cmd[++(*i)], 5)) == NULL)
            return (NULL);
    } else if (strcmp(cmd[*i], "|") == 0) {
        if ((lst = my_put_in_list(lst, cmd[*i], -1)) == NULL)
            return (NULL);
    } else {
        if ((lst = my_put_in_list(lst, cmd[*i], 1)) == NULL)
            return (NULL);
    }
    return (lst);
}

t_cmd *create_list(char **cmd)
{
    int i;
    t_cmd *lst;

    if ((lst = init_list()) == NULL)
        return (NULL);
    i = 0;
    while (cmd[i]) {
        if (strcmp(cmd[i], ">") == 0) {
            if ((lst = my_put_in_list(lst, cmd[++i], 4)) == NULL)
                return (NULL);
        } else if (strcmp(cmd[i], "<") == 0) {
            if ((lst = my_put_in_list(lst, cmd[++i], 3)) == NULL)
                return (NULL);
        } else if ((lst = do_check_param(lst, cmd, &i)) == NULL)
            return (NULL);
        i++;
    }
    return (lst);
}
