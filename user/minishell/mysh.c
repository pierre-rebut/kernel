/*
** mysh.c for mysh in /home/rebut_p/Programmation/Projet/minishell/src
** 
** Made by rebut_p
** Login   <rebut_p@epitech.net>
** 
** Started on  Tue Jan  6 11:41:07 2015 rebut_p
** Last update Sun Feb  1 12:11:31 2015 rebut_p
*/


#include <kstd.h>
#include <stdio.h>
#include <string.h>

#include "mysh.h"

char **my_found_path(const char **env) {
    char *search_path;
    char **my_path;

    search_path = my_getenv(env, "PATH");
    if (search_path == NULL)
        return (NULL);
    my_path = my_wordtab(search_path, 0, ',');
    free(search_path);
    return (my_path);
}

char *my_search_file_path(char **path, const char **av) {
    struct stat sb;
    char *tmp;
    char *str;
    int i;

    i = -1;
    if (path == NULL)
        return NULL;

    while (path[++i]) {
        tmp = strcat(path[i], "/");
        str = strcat(tmp, av[0]);
        free(tmp);
        if (stat(str, &sb) != -1)
            return (str);
        free(str);
    }

    return NULL;
}

int my_other_commande(const char **av, const char **env) {
    char **prg_path;

    if ((prg_path = my_found_path(env)) == NULL)
        puts("WARNING : PATH not set\n");

    const char *tmp = my_search_file_path(prg_path, av);
    if (tmp == NULL)
        tmp = strdup(av[0]);

    u32 pid = execve(tmp, av, env);
    if (pid == 0)
        printf("Error %s: no such file or directory\n", tmp);
    else
        waitpid(pid);

    do_free_path(prg_path, tmp);
    return (0);
}

int mysh(const char **parse_tab, char ***env) {
    if (strcmp(parse_tab[0], "help") == 0)
        builtin_help(parse_tab);
    else if (strcmp(parse_tab[0], "env") == 0)
        builtin_env(parse_tab, *env);
    else if (strcmp(parse_tab[0], "setenv") == 0) {
        if (parse_tab[1] != NULL && parse_tab[2] != NULL && parse_tab[3] != NULL)
            puts("ERROR : two much argument\n");
        else
            *env = builtin_setenv(parse_tab, *env, 0);
    } else if (strcmp(parse_tab[0], "unsetenv") == 0)
        builtin_unsetenv(parse_tab, *env);
    else if (strcmp(parse_tab[0], "cd") == 0)
        *env = builtin_cd(parse_tab, *env);
    else
        my_other_commande(parse_tab, *env);
    return (0);
}
