/*
** fdf_fct.c for fdf in /home/rebut_p/Programmation/Projet/MUL_2014_fdf
** 
** Made by rebut_p
** Login   <rebut_p@epitech.net>
** 
** Started on  Tue Oct 28 13:09:55 2014 rebut_p
** Last update Thu Jan 22 14:57:36 2015 rebut_p
*/

#include    "../../libs/include/stdlib.h"
#include    <unistd.h>
#include    "mysh.h"

int my_len_mots(const char *str, char param) {
    int i;
    int nb;

    i = 0;
    nb = 1;
    while (str[i] != '\0') {
        if (str[i] == param)
            nb++;
        i++;
    }
    return (nb);
}

int mlen(const char *str, int i, char param) {
    int nb;
    int pose;

    pose = 0;
    nb = 0;
    while (str[i] != '\0' && (str[i] != param || pose == 1)) {
        if (str[i] == '\"' && pose == 0)
            pose = 1;
        else if (str[i] == '\"')
            pose = 0;
        i++;
        nb++;
    }
    return (nb);
}

int my_str_to_wordtab(const char *str, int b, char param, t_tab *ml_t) {
    int pose;
    int j;

    j = 0;
    pose = 0;
    while (str[ml_t->i] != '\0' && (str[ml_t->i] != param || pose == 1)) {
        if (str[ml_t->i] == '\"' && pose == 0)
            pose = 1;
        else if (str[ml_t->i] == '\"')
            pose = 0;
        else
            ml_t->tab[b][j++] = str[ml_t->i];
        ml_t->i++;
    }
    if (str[ml_t->i] == '\0' && pose == 1)
        return (-1);
    ml_t->tab[b][j] = '\0';
    return (0);
}

char **my_wordtab(const char *str, int b, char param) {
    int size;
    t_tab ml_t;

    ml_t.i = 0;
    ml_t.tab = my_malloc(sizeof(char *) * (my_len_mots(str, param) + 2));
    while (str[ml_t.i] != '\0') {
        if (str[ml_t.i] != param) {
            size = mlen(str, ml_t.i, param) + 2;
            ml_t.tab[b] = my_malloc(sizeof(char) * size);
            if (my_str_to_wordtab(str, b, param, &ml_t) == -1)
                return (NULL);
            b++;
        } else
            ml_t.i++;
    }
    if (b == 0)
        return (NULL);
    ml_t.tab[b] = NULL;
    return (ml_t.tab);
}
