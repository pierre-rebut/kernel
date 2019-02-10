/*
** builtins.c for 42sh in /home/despla_s/Epitech/Systeme_Unix/PSU_2014_42sh/src
**
** Made by despla_s
** Login   <despla_s@epitech.net>
**
** Started on  Tue May 19 14:29:01 2015 despla_s
** Last update Sun May 24 18:50:10 2015 despla_s
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "functions.h"
#include "define.h"

int cd_call(char *path, t_env *env, int fd) {
    char *old_path;
    char *new_path;

    old_path = my_getenv("PWD", env->env);

    if ((new_path = cd_check_arg(path, env, fd)) == NULL)
        return (FAIL);

    setenv_call("OLDPWD", old_path, env);
    setenv_call("PWD", new_path, env);

    free(new_path);
    return (SUCCESS);
}

int env_call(t_env *env, int fd) {
    int i;

    i = 0;
    if (env->env == NULL)
        return (FAIL);
    while (env->env[i] != NULL) {
        if (my_putfd(env->env[i], fd) == FAIL) {
            printf("42sh : access denied\n");
            return (FAIL);
        }
        if (my_putfd("\n", fd) == FAIL)
            return (FAIL);
        i++;
    }
    return (SUCCESS);
}

int setenv_call(char *name, char *var, t_env *env) {
    int i;
    char *new_env_var;

    i = 0;
    if (name == NULL) {
        printf("Usage : setenv NAME [VAR]\n");
        return (FAIL);
    }
    new_env_var = strcat(name, "=");
    if (var != NULL)
        new_env_var = strcat(new_env_var, var);
    while (env->env[i] != NULL) {
        if (strncmp(name, env->env[i], strlen(name)) == 0) {
            modify_var(name, new_env_var, env);
            return (0);
        }
        i++;
    }
    add_var(new_env_var, env);
    if (new_env_var != NULL)
        free(new_env_var);
    return (SUCCESS);
}

int unsetenv_call(char *var, t_env *env) {
    char **new_env;
    int lines;
    int line_cpy;

    if (var == NULL) {
        printf("Usage : unsetenv NAME\n");
        return (FAIL);
    }
    if ((line_cpy = my_getenv_line(var, env->env)) == -1) {
        printf("%s doesn't exist\n", var);
        return (FAIL);
    }
    lines = 0;
    while (env->env[lines] != NULL)
        lines++;
    if ((new_env = malloc(sizeof(char *) * (lines + 2))) == NULL)
        return (FAIL);
    remove_var(new_env, line_cpy, env);
    return (SUCCESS);
}

int echo(t_cmd *cmd, int fd) {
    char bool;
    int i;

    i = 0;
    bool = 0;
    if (cmd->args[1] == NULL)
        return (my_putfd("\n", fd));
    if (strcmp(cmd->args[1], "-n") == SUCCESS) {
        i++;
        bool = 1;
    }
    while (cmd->args[++i]) {
        if (my_putfd(cmd->args[i], fd) == FAIL) {
            printf("42sh : access denied\n");
            return (FAIL);
        }
        if (cmd->args[i + 1] != NULL)
            my_putfd(" ", fd);
    }
    if (bool == 0)
        my_putfd("\n", fd);
    return (SUCCESS);
}
