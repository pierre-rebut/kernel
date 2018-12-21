/*
** my_getenv.c for my_getenv in /home/rebut_p/Programmation/Projet/my_select
** 
** Made by rebut_p
** Login   <rebut_p@epitech.net>
** 
** Started on  Sat Jan  3 18:27:36 2015 rebut_p
** Last update Tue Jan 20 16:40:04 2015 rebut_p
*/

#include <kstd.h>
#include <string.h>
#include <stdio.h>
#include "mysh.h"

int define_term(const char **env, const char *type, int cpt, int i) {
    char *test;

    while (env[cpt]) {
        i = 0;
        while (env[cpt][i] && env[cpt][i] != '=')
            i++;
        if ((test = malloc(sizeof(char) * (i + 1))) == NULL) {
            puts("Error: can't alloc memory\n");
            exit(1);
        }
        i = -1;
        while (env[cpt][++i] && env[cpt][i] != '=')
            test[i] = env[cpt][i];
        test[i] = '\0';
        if (strcmp(test, type) == 0) {
            free(test);
            return (cpt);
        }
        free(test);
        cpt++;
    }
    return (-1);
}

char *my_getenv(const char **env, char *type) {
    int i;
    int cpt;
    char *resp;

    i = 0;
    if ((cpt = define_term(env, type, 0, 0)) == -1)
        return (NULL);
    while (env[cpt][i] && env[cpt][i] != '=')
        i++;
    resp = strdup(env[cpt] + i + 1);
    return (resp);
}
