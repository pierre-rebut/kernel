/*
** my_ls.c for my_ls in /home/rebut_p/Programmation/Projet/my_ls
** 
** Made by rebut_p
** Login   <rebut_p@epitech.net>
** 
** Started on  Tue Nov 18 21:44:18 2014 rebut_p
** Last update Sun Nov 30 17:43:34 2014 rebut_p
*/

#include <kstd.h>
#include <stdio.h>
#include "include/my_ls.h"

int set_zero(t_option *opt) {
    opt->all = 0;
    opt->ll = 0;
    opt->rr = 0;
    opt->dir = 0;
    opt->RR = 0;
    opt->time = 0;
    return (0);
}

int set_option(t_option *opt, char **av) {
    int i;
    int u;

    i = -1;
    while (av[++i] != NULL)
        if (av[i][0] == '-' && ((u = 0) == 0))
            while (av[i][++u] != '\0') {
                if (av[i][u] == 'a')
                    opt->all = 1;
                else if (av[i][u] == 'l')
                    opt->ll = 1;
                else if (av[i][u] == 'r')
                    opt->rr = 1;
                else if (av[i][u] == 'd')
                    opt->dir = 1;
                else if (av[i][u] == 'R')
                    opt->RR = 1;
                else if (av[i][u] == 't')
                    opt->time = 1;
                else
                    return (1);
            }
    return (0);
}

int print_ls(t_option *opt, char *dir) {
    int total;
    char **tab;
    int dirp;
    int nb_file;

    total = 0;
    nb_file = 0;
    if ((dirp = opendir(dir)) == -1)
        return (do_file(opt, dir));
    while (readdir(dirp) != NULL)
        nb_file++;
    if ((dirp = opendir(dir)) == -1)
        return (do_file(opt, dir));
    tab = do_ls_allon(opt, dirp, nb_file);
    total = get_total_ll(opt, tab, dir);
    if (opt->ll == 1)
        printf("total %d\n", total);
    set_sort(opt, tab, dir);
    closedir(dirp);
    return (1);
}

int do_ls(t_option *opt, char **av) {
    int i;
    int nb;

    if (set_option(opt, av) == 1)
        return (1);
    if ((nb = get_nbr_dir(av)) == 0)
        if (set_opt_dd(opt, ".") == 0)
            print_ls(opt, ".");
    i = 0;
    while (av[++i] != NULL) {
        if (av[i][0] != '-') {
            if (nb > 1)
                printf("%s:\n", av[i]);
            if (set_opt_dd(opt, av[i]) == 1);
            else if (print_ls(opt, av[i]) == 0)
                puts("Error2: no file or directory\n");
            nb--;
            if (nb > 0)
                puts("\n");
        }
    }
    return (0);
}

int main(int ac, char **av) {
    char **tab;
    int dirp;
    int i;
    t_option opt;

    set_zero(&opt);
    if (ac != 1)
        verife_option(&opt, av);
    else {
        if ((dirp = opendir(".")) == -1)
            puts("Error1: no file or directory\n");
        else {
            tab = do_ls_allon(&opt, dirp, compte_nbfile("."));
            i = -1;
            while (tab[++i])
                if (tab[i][0] != '.' && tab[i][0] != '#')
                    printf("%s\n", tab[i]);
            closedir(dirp);
            do_free(tab);
        }
    }
    return (0);
}
