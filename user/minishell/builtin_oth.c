/*
** builtin_oth.c for mysh in /home/rebut_p/Programmation/Projet/sys_unix/minishell1/src
** 
** Made by rebut_p
** Login   <rebut_p@epitech.net>
** 
** Started on  Wed Jan 21 15:18:50 2015 rebut_p
** Last update Thu Jan 29 14:41:16 2015 rebut_p
*/

#include    <stdlib.h>
#include <string.h>
#include <stdio.h>
#include    "mysh.h"

char **cd_change_pwd(const char **parse_tab, char **env) {
    char *home;

    if (strcmp(parse_tab[1], "-") == 0) {
        home = my_getenv(env, "OLDPWD");
        if (home == NULL) {
            puts("ERROR : OLDPWD not found in env\n");
            return (env);
        }
        env = change_dir(home, env);
        free(home);
    } else
        env = change_dir(parse_tab[1], env);
    return (env);
}

char **builtin_cd(const char **parse_tab, char **env) {
    char *home;

    if (parse_tab[1] != NULL) {
        if (parse_tab[2] == NULL)
            env = cd_change_pwd(parse_tab, env);
        else
            puts("ERROR : two much arument\n");
    } else {
        home = my_getenv(env, "HOME");
        if (home == NULL) {
            puts("ERROR : HOME not found in env\n");
            return (env);
        }
        env = change_dir(home, env);
        free(home);
    }
    return (env);
}
