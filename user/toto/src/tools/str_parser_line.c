/*
** str_parser_line.c for parser in /home/rebut_p/Programmation/Projet-b2/sys_unix/42sh/src/tools
**
** Made by Pierre REBUT
** Login   <rebut_p@epitech.net>
**
** Started on  Mon May 18 13:38:25 2015 Pierre REBUT
** Last update Tue May 19 14:24:52 2015 Pierre REBUT
*/

#include <stdlib.h>

int count_nb_line(const char *str, int i) {
    int num;

    num = 1;
    while (str[i]) {
        if (str[i] == ';')
            num += 2;
        else if (str[i] == '&' && str[i + 1] == '&') {
            if (str[i + 2] == '|' || str[i + 2] == '&' || str[i + 2] == ';')
                return (-42);
            num += 2;
            i++;
        } else if (str[i] == '|' && str[i + 1] == '|') {
            if (str[i + 2] == '|' || str[i + 2] == '&' || str[i + 2] == ';')
                return (-42);
            num += 2;
            i++;
        }
        i++;
    }
    return (num);
}

int count_word(const char *str, int i) {
    int num;

    num = 1;
    while (str[i]) {
        if ((str[i] == ';') ||
            (str[i] == '&' && str[i + 1] == '&') ||
            (str[i] == '|' && str[i + 1] == '|')) {
            return (num);
        }
        num++;
        i++;
    }
    return (num);
}

int str_add_comand(char **tab, char *str, int i, int *y) {
    int x;
    char bool;

    x = 0;
    bool = 0;
    tab[*y] = malloc(sizeof(char) * (count_word(str, i) + 2));
    if (tab[*y] == NULL)
        return (-1);
    while (bool != 1) {
        if (str[i] == '\0')
            bool = 1;
        else if ((str[i] == ';') ||
                 (str[i] == '&' && str[i + 1] == '&') ||
                 (str[i] == '|' && str[i + 1] == '|'))
            bool = 1;
        else
            tab[*y][x++] = str[i++];
    }
    tab[*y][x] = '\0';
    (*y)++;
    return (i);
}

int get_separ(char **tab, const char *str, int i, int *y) {
    int x;

    x = 0;
    if ((tab[*y] = malloc(sizeof(char) * 3)) == NULL)
        return (-1);
    tab[*y][x++] = str[i++];
    if (str[i] == '&' || str[i] == '|')
        tab[*y][x++] = str[i++];
    tab[*y][x] = '\0';
    (*y)++;
    return (i);
}

char **str_parser_line(char *str, int i, int y) {
    int len;
    char **tab;

    if ((len = count_nb_line(str, 0)) == -42)
        return (NULL);
    if ((tab = malloc(sizeof(char *) * (len + 3))) == NULL)
        return (NULL);
    if (get_separ(tab, ";", 0, &y))
        while (str[i] != '\0') {
            if ((str[i] == ';') ||
                (str[i] == '&' && str[i + 1] == '&') ||
                (str[i] == '|' && str[i + 1] == '|')) {
                if ((i = get_separ(tab, str, i, &y)) == -1)
                    return (NULL);
            } else {
                if ((i = str_add_comand(tab, str, i, &y)) == -1)
                    return (NULL);
            }
        }
    tab[y] = NULL;
    return (tab);
}
