/*
** check.c for 42sh in /home/ganive_v/rendu/systeme_unix/PSU_2014_42sh/src/parsing
**
** Made by Vincent Ganivet
** Login   <ganive_v@epitech.net>
**
** Started on  Wed May  6 17:45:00 2015 Vincent Ganivet
** Last update Sun May 24 15:08:26 2015 despla_s
*/

#include <stdlib.h>

#include "define.h"
#include "functions.h"

int check_redir(const char *str)
{
    if (str[0] == '<' && str[1] == '<' && str[2] == '\0')
        return (3);
    else if (str[0] == '<' && str[1] == '\0')
        return (2);
    else if (str[0] == '>' && str[1] == '>' && str[2] == '\0')
        return (4);
    else if (str[0] == '>' && str[1] == '\0')
        return (4);
    else
        return (FAIL);
}

int check_r_and_f(char *str)
{
    int redir;
    int i;
    char *tmp;

    i = 0;
    while ((str[i] == '>' || str[i] == '<') && str[i] != '\0')
        i++;
    tmp = my_strndup(str, i);
    if ((redir = check_redir(tmp)) == FAIL) {
        free(tmp);
        return (FAIL);
    }
    free(tmp);
    tmp = (strdup_to_char(str + i, "\0"));
    tmp = (strdup_to_char(tmp, " "));

    if (tmp[0] == '\0') {
        free(tmp);
        return (FAIL);
    }

    free(tmp);
    return (redir);
}

int check_binary(char *binary)
{
    char *tmp;

    tmp = strdup_to_char(binary, "><\0");
    if (tmp[0] == '\0')
        return (FAIL);
    free(tmp);
    return (SUCCESS);
}
