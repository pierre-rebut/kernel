/*
** my_ls.h for my_ls in /home/rebut_p/Programmation/Projet/my_ls
** 
** Made by rebut_p
** Login   <rebut_p@epitech.net>
** 
** Started on  Tue Nov 18 21:51:41 2014 rebut_p
** Last update Sun Nov 30 17:43:24 2014 rebut_p
*/

#ifndef MY_LS_H_
# define MY_LS_H_

#include "../../../libs/include/types.h"

typedef struct s_option {
    int all;
    int dir;
    int ll;
    int rr;
    int RR;
    int time;
} t_option;

int do_sort_file(char *str1, char *str2);

int compte_nbfile(char *str);

int do_file(t_option *opt, char *file);

int set_opt_dd(t_option *opt, char *dir);

char *my_directory2(char *name);

int do_free(char **tab);

int verife_option(t_option *opt, char **av);

int format_ll2(char *file, char *name);

int format_ll(char *entry, char *dir);

int set_sort(t_option *opt, char **tab, char *dir);

int do_sort(t_option *opt, char **tab);

int my_swap(char **tab, int i, int *bool);

int len_tab(char **tab);

int get_nbr_dir(char **av);

int get_total_ll(t_option *opt, char **tab, char *dir);

char *my_directory(char *address, char *name);

char **do_ls_allon(t_option *opt, int dirp, int nb_file);

#endif
