/*
** main.c for mysh in /home/rebut_p/Programmation/Projet/minishell/src
** 
** Made by rebut_p
** Login   <rebut_p@epitech.net>
** 
** Started on  Tue Jan  6 11:40:14 2015 rebut_p
** Last update Thu Jan 22 15:51:42 2015 rebut_p
*/

#include    <stdio.h>
#include    <string.h>
#include    <stdlib.h>
#include    "mysh.h"

int do_free_tab(const char **tab) {
    int i;

    i = 0;
    if (tab != NULL) {
        while (tab[i])
            free(tab[i++]);
    }
    free(tab);
    return (0);
}

int do_mysh(const char **parse_tab, char ***env, char *my_commande) {
    if (strcmp(parse_tab[0], "exit") == 0)
        do_exit(my_commande, parse_tab, *env);
    else
        mysh(parse_tab, env);
    return (0);
}

int main(int ac, char **av, const char **envo) {
    char **env;
    char *my_commande;

    env = copie_environemnt(envo, ac);
    (void) av;
    while (42) {
        puts("MySh $> ");
        my_commande = get_next_line(0);
        if (my_commande == NULL)
            return do_oth_exit(my_commande);
        if (my_commande[0] != '\0') {
            const char **parse_tab = (const char**) my_wordtab(my_commande, 0, ' ');
            if (parse_tab != NULL)
                do_mysh(parse_tab, &env, my_commande);
            else
                puts("ERROR : incorecte commande\n");
            do_free_tab(parse_tab);
        }
        free(my_commande);
    }
}
