/*
** tools.c for 42sh in /home/despla_s/Epitech/Systeme_Unix/PSU_2014_42sh/src/tools
**
** Made by despla_s
** Login   <despla_s@epitech.net>
**
** Started on  Wed May  6 14:38:24 2015 despla_s
** Last update Sun May 24 16:17:45 2015 despla_s
*/

#include <stdlib.h>
#include <string.h>

#include "functions.h"

int tab_len(char **tab) {
    int i;

    i = 0;
    while (tab[i])
        i++;
    return (i);
}

char **create_env(char **tab) {
    char **ret;
    int i;
    int j;

    if ((ret = malloc(sizeof(char *) * (tab_len(tab) + 2))) == NULL)
        return (NULL);
    i = 0;
    while (tab[i]) {
        if ((ret[i] = malloc(sizeof(char) * (strlen(tab[i]) + 1))) == NULL)
            return (NULL);
        j = 0;
        while (tab[i][j]) {
            ret[i][j] = tab[i][j];
            j++;
        }
        ret[i][j] = '\0';
        i++;
    }
    ret[i] = NULL;
    return (ret);
}
