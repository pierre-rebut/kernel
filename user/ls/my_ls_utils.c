/*
** my_ls_utils.c for my_ls in /home/rebut_p/Programmation/Projet/my_ls
** 
** Made by rebut_p
** Login   <rebut_p@epitech.net>
** 
** Started on  Wed Nov 26 10:54:46 2014 rebut_p
** Last update Sat Nov 29 14:35:28 2014 rebut_p
*/

#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "kstd.h"


#include "include/my_ls.h"

int len_tab(char **tab) {
    int i;

    i = 0;
    while (tab[i])
        i++;
    return (i);
}

int my_swap(char **tab, int i, int *bool) {
    char *tmp;

    tmp = tab[i + 1];
    tab[i + 1] = tab[i];
    tab[i] = tmp;
    *bool = 1;
    return (0);
}

char **do_ls_allon(t_option *opt, DIR *dirp, int nb_file) {
    char **tab;
    struct dirent *entry;
    int i;
    int j;

    if ((tab = malloc(sizeof(char *) * (nb_file + 1))) == NULL)
        return (0);
    i = 0;
    while ((entry = readdir(dirp)) != NULL) {
        j = -1;
        if ((tab[i] = malloc(sizeof(char) * (strlen(entry->d_name)) + 1))
            == NULL)
            return (NULL);
        while (entry->d_name[++j] != '\0')
            tab[i][j] = entry->d_name[j];
        tab[i++][j] = '\0';
    }
    tab[i] = NULL;
    do_sort(opt, tab);
    return (tab);
}

int get_total_ll(t_option *opt, char **tab, char *dir) {
    struct stat sb;
    int i;
    char *direc;
    int total;

    i = 0;
    total = 0;
    while (tab[i]) {
        direc = my_directory(dir, tab[i]);
        if (stat(direc, &sb) == -1)
            return (0);
        if (opt->all == 0 && tab[i][0] != '.')
            total += (sb.st_size / 2);
        else if (opt->all == 1)
            total += (sb.st_size / 2);
        free(direc);
        i++;
    }
    return (total);
}
