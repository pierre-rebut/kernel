/*
** mysh_fct_utils.c for mysh in /home/rebut_p/Programmation/Projet/sys_unix/minishell1
** 
** Made by rebut_p
** Login   <rebut_p@epitech.net>
** 
** Started on  Wed Jan 21 11:15:20 2015 rebut_p
** Last update Sun Feb  1 12:07:10 2015 rebut_p
*/

#include <kstd.h>
#include <string.h>
#include <stdio.h>
#include "mysh.h"

int builtin_help(const char **parse_tab) {
    if (parse_tab[1] != NULL)
        puts("ERROR : incorrecte option\n");
    else {
        puts("HELP :\n - ((expression))\n - cd [-] [dir]\n" \
          " - setenv NAME env\n - unsetenv " \
          "NAME\n - env\n - help\n - exit\n");
    }
    return (0);
}

char **do_tab_setenv(char *name, char *param) {
    char **toto;

    toto = my_malloc(sizeof(char *) * 4);
    toto[0] = strdup("setenv");
    toto[1] = strdup(name);
    toto[2] = strdup(param);
    toto[3] = NULL;
    return (toto);
}

char **change_dir(const char *directory, char **env) {
    char **toto;
    char *test;

    if (chdir(directory) == -1) {
        puts("ERROR : no such file or directory\n");
        return (env);
    }
    test = my_getenv(env, "PWD");
    if (test != NULL) {
        toto = do_tab_setenv("OLDPWD", test);
        free(test);
        env = builtin_setenv(toto, env, 0);
        do_free_tab(toto);
    }
    toto = do_tab_setenv("PWD", directory);
    env = builtin_setenv(toto, env, 0);
    do_free_tab(toto);
    return (env);
}

int do_oth_exit(char *buff) {
    free(buff);
    puts("exit\n");
    exit(1);
    return (0);
}
