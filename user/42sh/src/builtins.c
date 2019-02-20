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
#include <unistd.h>
#include <alloc.h>

#include "functions.h"
#include "define.h"

static int my_setenv(char *name, char *value, t_env *env)
{
    int i;
    char *new_env_var;

    i = 0;
    if (name == NULL) {
        printf("Usage : setenv NAME [VAR]\n");
        return FAIL;
    }

    new_env_var = strcat(name, "=");
    if (value != NULL)
        new_env_var = strcat(new_env_var, value);
    while (env->env[i] != NULL) {
        if (strncmp(name, env->env[i], strlen(name)) == 0) {
            modify_var(name, new_env_var, env);
            return SUCCESS;
        }
        i++;
    }
    add_var(new_env_var, env);
    if (new_env_var != NULL)
        free(new_env_var);

    return SUCCESS;
}

int cd_call(int fd, t_cmd *cmd, t_env *env)
{
    char *old_path;
    char *new_path;

    old_path = my_getenv("PWD", env->env);

    if ((new_path = cd_check_arg(cmd->args[1], env, fd)) == NULL)
        return (FAIL);

    my_setenv("OLDPWD", old_path, env);
    my_setenv("PWD", new_path, env);

    free(new_path);
    return SUCCESS;
}

int env_call(int fd, t_cmd *cmd, t_env *env)
{
    (void) cmd;

    int i;

    i = 0;
    if (env->env == NULL)
        return FAIL;

    while (env->env[i] != NULL) {
        my_putfd(env->env[i], fd);
        my_putfd("\n", fd);
        i++;
    }
    return SUCCESS;
}

int setenv_call(int fd, t_cmd *cmd, t_env *env)
{
    (void) fd;

    char *name = cmd->args[1], *value = cmd->args[2];
    return my_setenv(name, value, env);
}

int unsetenv_call(int fd, t_cmd *cmd, t_env *env)
{
    (void) fd;

    char *var = cmd->args[1];

    char **new_env;
    int lines;
    int line_cpy;

    if (var == NULL) {
        printf("Usage : unsetenv NAME\n");
        return FAIL;
    }
    if ((line_cpy = my_getenv_line(var, env->env)) == -1) {
        printf("%s doesn't exist\n", var);
        return FAIL;
    }
    lines = 0;
    while (env->env[lines] != NULL)
        lines++;
    if ((new_env = malloc(sizeof(char *) * (lines + 2))) == NULL)
        return FAIL;
    remove_var(new_env, line_cpy, env);

    return SUCCESS;
}

int echo(int fd, t_cmd *cmd, t_env *env)
{
    (void) env;

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
        my_putfd(cmd->args[i], fd);
        if (cmd->args[i + 1] != NULL)
            my_putfd(" ", fd);
    }
    if (bool == 0)
        my_putfd("\n", fd);

    return SUCCESS;
}

int sync_call(int fd, t_cmd *cmd, t_env *env)
{
    (void) fd;
    (void) cmd;
    (void) env;

    sync();
    return SUCCESS;
}