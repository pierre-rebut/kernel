/*
** my_ls_utils2.c for my_ls in /home/rebut_p/Programmation/Projet/my_ls
** 
** Made by rebut_p
** Login   <rebut_p@epitech.net>
** 
** Started on  Wed Nov 26 13:43:42 2014 rebut_p
** Last update Sun Nov 30 17:42:58 2014 rebut_p
*/

#include <stdlib.h>
#include "kstd.h"
#include <stdio.h>
#include "include/my_ls.h"

int get_nbr_dir(char **av)
{
    int i;
    int nb;

    i = 1;
    nb = 0;
    while (av[i]) {
        if (av[i][0] != '-')
            nb++;
        i++;
    }
    return (nb);
}

int do_sort_file(char *str1, char *str2)
{
    int i;
    char c;
    char d;

    i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] == str2[i] || (str1[i] == '.' && str2[i] == '_'))
            i++;
        else {
            c = str1[i];
            d = str2[i];
            free(str1);
            free(str2);
            return (c - d);
        }
    }
    c = str1[i];
    d = str2[i];
    free(str1);
    free(str2);
    return (c - d);
}

int compte_nbfile(char *str)
{
    int nb;
    DIR *dirp;

    if ((dirp = opendir(str)) == NULL)
        return (-1);
    nb = 0;
    while (readdir(dirp) != NULL)
        nb++;
    closedir(dirp);
    return (nb);
}

int do_ls(t_option *opt, char **av);

int verife_option(t_option *opt, char **av)
{
    if (do_ls(opt, av) == 1)
        puts("Error option\n");
    return (0);
}
