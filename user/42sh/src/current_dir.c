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
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "functions.h"
#include "define.h"

char *cd_tild(t_env *env)
{
    char *home;

    if ((home = (my_getenv("HOME", env->env))) == NULL) {
        printf("cd: HOME not set\n");
        return (NULL);
    }
    if (chdir(home) == -1) {
        printf("cd: '%s': %s\n", home, strerror(errno));
        return (NULL);
    }
    return (strdup(home));
}

char *cd_dash(t_env *env, int fd)
{
    char *oldpwd = my_getenv("OLDPWD", env->env);

    if (oldpwd == NULL) {
        printf("cd: OLDPWD not set or doesn't exist\n");
        return (NULL);
    }
    if (chdir(oldpwd) == -1) {
        printf("cd: '%s': %s\n", oldpwd, strerror(errno));
        return (NULL);
    }
    my_putfd(oldpwd, fd);
    my_putfd("\n", fd);
    return (strdup(oldpwd));
}

char *cd_check_arg(char *path, t_env *env, int fd)
{
    char *new_path = my_getenv("PWD", env->env);

    if (path == NULL || strcmp("~", path) == SUCCESS) {
        new_path = cd_tild(env);
    } else if (strcmp(path, "-") == SUCCESS) {
        new_path = cd_dash(env, fd);
    } else {
        if (chdir(path) == FAIL) {
            printf("cd: '%s': %s\n", path, strerror(errno));
            return (NULL);
        }
        new_path = strdup(new_path);
        new_path = strcat(new_path, path);
    }

    return (new_path);
}
