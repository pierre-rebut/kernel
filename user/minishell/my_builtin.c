/*
** my_builtin.c for mysh in /home/rebut_p/Programmation/Projet/sys_unix/minishell1
** 
** Made by rebut_p
** Login   <rebut_p@epitech.net>
** 
** Started on  Tue Jan 20 11:04:48 2015 rebut_p
** Last update Thu Jan 29 13:23:27 2015 rebut_p
*/

#include    "../../libs/include/stdlib.h"
#include <string.h>
#include <stdio.h>
#include    "mysh.h"

int builtin_env(const char **parse_tab, char **env) {
    int i;

    i = 0;
    if (parse_tab[1] != NULL) {
        puts("ERROR : two much argument\n");
        return (0);
    }
    while (env[i]) {
        if (env[i][0] != '\0') {
            printf("%s\n", env[i]);
        }
        i++;
    }
    return (0);
}

int check_tab_equal(const char *new) {
    int i;

    i = 0;
    while (new[i] != '\0') {
        if (new[i] == '=')
            return (-1);
        else
            i++;
    }
    return (0);
}

char **add_setenv(char **env, char **tab) {
    int i;
    char *tmp;
    char *new;
    char **new_env;

    i = -1;
    tmp = strcat(tab[1], "=");
    new = strcat(tmp, tab[2]);
    free(tmp);
    new_env = my_malloc(sizeof(char *) * (len_env((const char **)env) + 2));
    while (env[++i])
        new_env[i] = env[i];
    new_env[i++] = strdup(new);
    new_env[i] = NULL;
    free(env);
    free(new);
    return (new_env);
}

char **builtin_setenv(const char **tab, char **env, int cpt) {
    char *tmp;
    char *new;

    if (tab[1] == NULL || tab[2] == NULL)
        puts("ERROR : two fee argument\n");
    else {
        if (check_tab_equal(tab[1]) == -1)
            puts("Error : this is not a setenv (can't have = arguments)\n");
        else {
            if ((cpt = define_term((const char**)env, tab[1], 0, 0)) == -1)
                env = add_setenv(env, tab);
            else {
                tmp = strcat(tab[1], "=");
                new = strcat(tmp, tab[2]);
                free(tmp);
                free(env[cpt]);
                env[cpt] = strdup(new);
                free(new);
            }
        }
    }
    return (env);
}

int builtin_unsetenv(const char **tab, char **env) {
    int cpt;

    if (tab[1] == NULL) {
        puts("ERROR : two fee argument\n");
        return (0);
    }
    if (tab[2] != NULL) {
        puts("ERROR : two much argument\n");
        return (0);
    }
    if ((cpt = define_term(env, tab[1], 0, 0)) == -1) {
        puts("ERROR : PARAM not found in env\n");
        return (0);
    }
    free(env[cpt]);
    env[cpt] = my_malloc(sizeof(char) * 2);
    env[cpt][0] = '\0';
    return (0);
}
