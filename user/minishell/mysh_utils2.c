/*
** mysh_utils2.c for mysh in /home/rebut_p/Programmation/Projet/minishell/src
** 
** Made by rebut_p
** Login   <rebut_p@epitech.net>
** 
** Started on  Fri Jan  9 00:05:05 2015 rebut_p
** Last update Thu Jan 22 15:51:36 2015 rebut_p
*/

#include "../../libs/include/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "mysh.h"

int len_env(const char **env) {
    int i;

    i = 0;
    while (env[i])
        i++;
    return (i);
}

char **copie_environemnt(const char **environ, int ac) {
    int i;
    int j;
    char **env;

    if (ac != 1)
        puts("Parametre incorrecte\n");

    if (environ == NULL) {
        env = my_malloc(sizeof(char *) * 2);
        env = NULL;
        return (env);
    }
    i = -1;
    env = my_malloc(sizeof(char *) * (len_env(environ) + 1));
    while (environ[++i]) {
        env[i] = my_malloc(sizeof(char) * (strlen(environ[i]) + 1));
        j = -1;
        while (environ[i][++j] != '\0')
            env[i][j] = environ[i][j];
        env[i][j] = '\0';
    }
    env[i] = NULL;
    return (env);
}

int do_free_path(char **path, const char *tmp) {
    int i;

    i = 0;
    if (path != NULL) {
        while (path[i])
            free(path[i++]);
        free(path);
    }
    free(tmp);
    return (0);
}
