/*
** list_check.c for list in /home/rebut_p/Programmation/Projet-b2/sys_unix/42sh/src
**
** Made by Pierre REBUT
** Login   <rebut_p@epitech.net>
**
** Started on  Thu May 21 15:18:42 2015 Pierre REBUT
** Last update Thu May 21 15:25:50 2015 Pierre REBUT
*/

#include <stdlib.h>
#include <stdlib.h>

#include "struct.h"
#include "functions.h"

t_cmd *init_list()
{
    t_cmd *elem;

    elem = malloc(sizeof(t_cmd));
    if (elem == NULL)
        return (NULL);
    elem->pipefd[0] = 0;
    elem->pipefd[1] = 1;
    elem->prg = NULL;
    elem->args = NULL;
    elem->redir_out = NULL;
    elem->redir_in = NULL;
    elem->next = NULL;
    elem->prev = NULL;
    return (elem);
}
