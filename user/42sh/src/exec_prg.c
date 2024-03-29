/*
** exec_prg.c for execprg in /home/rebut_p/Programmation/Projet-b2/sys_unix/42sh/src
**
** Made by Pierre REBUT
** Login   <rebut_p@epitech.net>
**
** Started on  Thu May 21 15:31:59 2015 Pierre REBUT
** Last update Tue Jun  9 14:52:02 2015 dourch_m
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

#include "struct.h"
#include "define.h"
#include "functions.h"

char *my_getenv(char *str, char **env)
{
    int j;
    size_t i;

    j = -1;
    while (env[++j] != NULL) {
        if (strncmp(str, env[j], i = strlen(str)) == 0) {
            i++;
            return env[j] + i;
        }
    }
    return (NULL);
}

char *get_prg(char *prg, t_env *env)
{
    char *path;

    if ((path = my_getenv("PATH", env->env)) == NULL)
        return (NULL);

    path = strdup(path);
    path = strcat(path, "/");
    path = strcat(path, prg);

    if (stat(path, NULL) == -1) {
        printf("42sh: %s: %s\n", prg, strerror(errno));
        free(path);
        return NULL;
    }

    return path;
}

int exec_prg(struct ExceveInfo *execInfo, t_cmd *lst, t_env *env)
{
    if (lst->prg[0] != '/' && lst->prg[0] != '.')
        execInfo->cmdline = get_prg(lst->prg, env);
    else
        execInfo->cmdline = strdup(lst->prg);

    if (execInfo->cmdline == NULL) {
        return ERROR_FILS;
    }

    int pid = execve(execInfo);
    if (pid < 0) {
        printf("42sh: %s: %s\n", execInfo->cmdline, strerror(errno));
    }

    free(execInfo->cmdline);
    return (pid < 0 ? ERROR_FILS : pid);
}
