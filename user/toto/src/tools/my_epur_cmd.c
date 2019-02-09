/*
** my_epur_cmd.c for 42sh in /home/rebut_p/Programmation/Projet-b2/sys_unix/42sh/src/tools
**
** Made by Pierre REBUT
** Login   <rebut_p@epitech.net>
**
** Started on  Fri May 15 17:16:33 2015 Pierre REBUT
** Last update Fri May 22 18:16:14 2015 Pierre REBUT
*/

#include <stdlib.h>
#include <string.h>
#include "functions.h"

int add_separ_line(char *new, const char *str, int *i, int cpt) {
    if (str[*i] != ';' && str[*i] != '&' && str[*i] != '>'
        && str[*i] != '<' && str[*i] != '|')
        new[cpt++] = str[*i];
    else {
        if ((str[*i] == '|' && str[*i + 1] == '|') ||
            (str[*i] == '&' && str[*i + 1] == '&') ||
            (str[*i] == '<' && str[*i + 1] == '<') ||
            (str[*i] == '>' && str[*i + 1] == '>')) {
            new[cpt++] = ' ';
            new[cpt++] = str[(*i)++];
            new[cpt++] = str[*i];
            new[cpt++] = ' ';
        } else if (str[*i] == ';' || str[*i] == '&' || str[*i] == '>'
                   || str[*i] == '<' || str[*i] == '|') {
            new[cpt++] = ' ';
            new[cpt++] = str[*i];
            new[cpt++] = ' ';
        }
    }
    return (cpt);
}

char *my_epur_cmd(char *str, int i) {
    int cpt;
    int len;
    char *new;

    cpt = 0;
    len = strlen(str);
    new = malloc(sizeof(char) * (len + 2));
    if (new == NULL)
        return (NULL);
    while (str[i] != '\0') {
        if (cpt >= len - 4) {
            len *= 2;
            new = realloc(new, len + 2);
            if (new == NULL)
                return (NULL);
        }
        cpt = add_separ_line(new, str, &i, cpt);
        i++;
    }
    new[cpt] = '\0';
    free(str);
    return (new);
}
