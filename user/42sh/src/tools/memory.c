/*
** memory.c for 42sh in /home/despla_s/Epitech/Systeme_Unix/PSU_2014_42sh/src/tools
**
** Made by despla_s
** Login   <despla_s@epitech.net>
**
** Started on  Wed May  6 17:02:40 2015 despla_s
** Last update Fri May 22 16:11:58 2015 despla_s
*/

#include <stdlib.h>
#include <alloc.h>

#include "struct.h"
#include "define.h"
#include "functions.h"

int free_list(t_cmd *lst)
{
    t_cmd *tmp;

    while (lst != NULL) {
        tmp = lst->next;
        free(lst->prg);
        free_tab(lst->args);
        free(lst->redir_in);
        free(lst->redir_out);
        free(lst);
        lst = tmp;
    }
    return (0);
}

int free_tab(char **tab)
{
    int i;

    if (tab == NULL)
        return (FAIL);
    i = 0;
    while (tab[i] != NULL) {
        free(tab[i]);
        i++;
    }
    free(tab);
    return (SUCCESS);
}
