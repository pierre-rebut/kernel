/*
** my_wordtab.c for 42sh in /home/rebut_p/Programmation/Projet-b2/sys_unix/42sh
**
** Made by Pierre REBUT
** Login   <rebut_p@epitech.net>
**
** Started on  Wed May 13 17:29:13 2015 Pierre REBUT
** Last update Thu May 21 16:50:08 2015 Pierre REBUT
*/

#include <stdlib.h>
#include <alloc.h>

int len_tab(const char *str, char param)
{
    int num;
    int i;

    i = 0;
    num = 1;
    while (str[i] != '\0') {
        if (str[i] == param)
            num++;
        i++;
    }
    return (num);
}

int len_word(const char *str, int i, char param)
{
    int len;

    len = 1;
    while (str[i] && str[i] != param) {
        i++;
        len++;
    }
    return (len);
}

char **my_wordtab(char *str, char param, int i, int y)
{
    int x;
    char **tab;

    if ((tab = malloc(sizeof(char *) * (len_tab(str, param) + 2))) == NULL)
        return (NULL);
    while (str[i] != '\0') {
        if (str[i] != param) {
            x = 0;
            tab[y] = malloc(sizeof(char) * (len_word(str, i, param) + 1));
            if (tab[y] == NULL)
                return (NULL);
            while (str[i] && str[i] != param)
                tab[y][x++] = str[i++];
            tab[y][x] = '\0';
            y++;
        } else
            i++;
    }
    tab[y] = NULL;
    return (tab);
}
