/*
** mysh_utils.c for mysh in /home/rebut_p/Programmation/Projet/minishell/src
** 
** Made by rebut_p
** Login   <rebut_p@epitech.net>
** 
** Started on  Thu Jan  8 23:36:54 2015 rebut_p
** Last update Thu Jan 22 14:28:42 2015 rebut_p
*/

#include <kstd.h>
#include <stdio.h>
#include <string.h>
#include "mysh.h"

void *my_malloc(size_t len) {
    void *ptr;

    ptr = malloc(len);
    if (ptr == NULL) {
        puts("Error: can't alloc memory\n");
        exit(1);
    }
    return (ptr);
}

int do_exit(char *str, const char **tab, char **env) {
    int i;

    i = 0;
    while (tab[i])
        free(tab[i++]);
    free(tab);
    free(str);
    i = 0;
    if (env != NULL) {
        while (env[i])
            free(env[i++]);
        free(env);
    }
    puts("exit\n");
    exit(1);
    return (0);
}
