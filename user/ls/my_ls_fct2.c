/*
** my_ls_fct2.c for my_ls in /home/rebut_p/Programmation/Projet/my_ls
** 
** Made by rebut_p
** Login   <rebut_p@epitech.net>
** 
** Started on  Tue Nov 25 17:40:19 2014 rebut_p
** Last update Sun Nov 30 15:25:01 2014 rebut_p
*/

#include <alloc.h>
#include <string.h>
#include <stdio.h>

#include "include/my_ls.h"

int set_opt_dd(t_option *opt, char *dir) {
    int i;
    char *direc;

    if (opt->dir == 1) {
        if (opt->ll == 1) {
            if ((direc = malloc(sizeof(char) * (strlen(dir) + 2))) == NULL)
                return (0);
            i = -1;
            while (dir[++i] != '\0')
                direc[i] = dir[i];
            direc[i] = '\0';
            format_ll2(direc, dir);
        } else
            printf("%s\n", dir);
        return (1);
    }
    return (0);
}

char *my_directory2(char *name) {
    int i;
    int o;
    char *direc;

    i = 0;
    o = 0;
    if ((direc = malloc(sizeof(char) * (strlen(name) + 3))) == NULL)
        return (NULL);
    if (name[0] != '/' && name[0] != '.') {
        direc[0] = '.';
        direc[1] = '/';
        o += 2;
    }
    while (name[i] != '\0')
        direc[o++] = name[i++];
    direc[o] = '\0';
    return (direc);
}

int do_free(char **tab) {
    int i;

    i = 0;
    while (tab[i]) {
        free(tab[i]);
        i++;
    }
    free(tab);
    return (0);
}

int set_sort(t_option *opt, char **tab, char *dir) {
    int i;

    i = 0;
    while (tab[i] != NULL) {
        if (opt->all == 1) {
            if (opt->ll == 1)
                format_ll(tab[i], dir);
            else
                printf("%s\n", tab[i]);
        } else if (tab[i][0] != '.' && tab[i][0] != '#') {
            if (opt->ll == 1)
                format_ll(tab[i], dir);
            else
                printf("%s\n", tab[i]);
        }
        i++;
    }
    do_free(tab);
    return (0);
}

int do_sort(t_option *opt, char **tab) {
    int bool;
    int i;
    int sizetab;

    bool = 1;
    sizetab = len_tab(tab) + 1;
    while (bool == 1 && (sizetab--) > 0) {
        bool = 0;
        i = -1;
        while (++i < sizetab - 1) {
            if (opt->rr == 0 && strcmp(tab[i], tab[i + 1]) > 0)
                my_swap(tab, i, &bool);
            else if (opt->rr == 1 && strcmp(tab[i], tab[i + 1]) < 0)
                my_swap(tab, i, &bool);
        }
    }
    return (0);
}
