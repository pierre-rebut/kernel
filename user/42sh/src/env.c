/*
** env.c for 42sh in /media/despla_s/2337a639-4d85-4f36-ade6-cdcedffd0cf2/despla_s/rendu/Systeme_Unix/PSU_2014_42sh/src
** 
** Made by despla_s
** Login   <despla_s@epitech.net>
** 
** Started on  Fri May 22 14:01:54 2015 despla_s
** Last update Sun May 24 16:17:12 2015 despla_s
*/

#include <stdlib.h>
#include <string.h>
#include <alloc.h>
#include "define.h"
#include "functions.h"

int my_getenv_line(char *var, char **env)
{
    int i;
    size_t j;
    int k;
    char *name;

    i = 0;
    while (env[i]) {
        if (strncmp(var, env[i], (j = strlen(var))) == 0 && env[i][j] == '=') {
            if ((name = malloc(sizeof(char *) * (strlen(env[i]) - j))) == NULL)
                return (FAIL);
            k = 0;
            while (env[i][j]) {
                name[k] = env[i][j];
                j++;
                k++;
            }
            name[k] = '\0';
            return (i);
        }
        i++;
    }
    return (FAIL);
}

int modify_var(char *name, char *new_var, t_env *env)
{
    int line_nbr;

    line_nbr = my_getenv_line(name, env->env);
    free(env->env[line_nbr]);
    if ((env->env[line_nbr] = malloc(sizeof(char) *
                                     (strlen(new_var) + 1))) == NULL)
        return (FAIL);
    env->env[line_nbr] = strdup(new_var);
    return (SUCCESS);
}

int add_var(char *new_env_var, t_env *env)
{
    char **new_env;
    int i;

    i = 0;
    while (env->env[i] != NULL)
        i++;
    if ((new_env = create_env(env->env)) == NULL)
        return (FAIL);
    new_env[i] = strdup(new_env_var);
    new_env[i + 1] = NULL;
    free_tab(env->env);
    env->env = new_env;
    return (SUCCESS);
}

int remove_var(char **new_env, int line_cpy, t_env *env)
{
    int lines;
    int lines_2;

    lines = 0;
    lines_2 = 0;
    while (env->env[lines] != NULL) {
        if (lines == line_cpy)
            lines++;
        if (env->env[lines] != NULL) {
            new_env[lines_2] = strdup(env->env[lines]);
            lines++;
            lines_2++;
        }
    }
    new_env[lines_2] = NULL;
    free_tab(env->env);
    env->env = new_env;
    return (SUCCESS);
}
