/*
** current_dir.c for 42sh in /home/despla_s/Epitech/Systeme_Unix/PSU_2014_42sh/src
**
** Made by despla_s
** Login   <despla_s@epitech.net>
**
** Started on  Tue May 19 14:29:14 2015 despla_s
** Last update Sun May 24 18:37:35 2015 despla_s
*/

#include <string.h>
#include <syscallw.h>
#include <stdio.h>

#include "functions.h"
#include "define.h"

char *cd_tild(t_env *env) {
    char *home;

    if ((home = (my_getenv("HOME", env->env))) == NULL) {
        printf("cd: HOME not set\n");
        return (NULL);
    }
    if (chdir(home) == -1) {
        printf("cd: '%s' doesn't exist\n", home);
        return (NULL);
    }
    return (home);
}

char *cd_dash(t_env *env, int fd) {
    char *oldpwd;

    if ((oldpwd = (my_getenv("OLDPWD", env->env))) == NULL) {
        printf("cd: OLDPWD not set or doesn't exist\n");
        return (NULL);
    }
    if (chdir(oldpwd) == -1) {
        printf("cd: '%s' doesn't exist\n", oldpwd);
        return (NULL);
    }
    my_putfd(oldpwd, fd);
    my_putfd("\n", fd);
    return (oldpwd);
}

char *cd_check_arg(char *path, t_env *env, int fd) {
    char *new_path = NULL;

    if (path == NULL || strcmp("~", path) == SUCCESS) {
        if ((new_path = cd_tild(env)) == NULL)
            return (NULL);
    } else if (strcmp(path, "-") == SUCCESS) {
        if ((new_path = cd_dash(env, fd)) == NULL)
            return (NULL);
    } else if (chdir(path) == FAIL) {
        printf("cd: '%s' : Aucun fichier ou dossier de ce type\n", path);
        return (NULL);
    }
    return (new_path);
}
