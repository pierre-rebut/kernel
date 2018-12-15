/*
** mysh.h for mysh in /home/rebut_p/Programmation/Projet/minishell/include
** 
** Made by rebut_p
** Login   <rebut_p@epitech.net>
** 
** Started on  Thu Jan  8 23:34:13 2015 rebut_p
** Last update Thu Jan 29 13:10:50 2015 rebut_p
*/

#ifndef        MYSH_H_
# define    MYSH_H_
# define    BUFFER_SIZE 4096

# include    <stdlib.h>

typedef struct s_tab {
    char **tab;
    int i;
} t_tab;

int do_free_tab(const char **tab);

int builtin_help(const char **parse_tab);

char **builtin_cd(const char **parse_tab, char **env);

char **change_dir(const char *directory, char **env);

int do_oth_exit(char *buff);

int len_env(const char **env);

int define_term(const char **env, const char *type, int cpt, int i);

int builtin_env(const char **parse_tab, char **env);

int do_free_path(char **path, char *tmp);

char **builtin_setenv(const char **tab, char **env, int cpt);

int builtin_unsetenv(const char **tab, char **env);

char *my_getenv(const char **env, char *type);

int mysh(const char **parse_tab, char ***env);

char *get_next_line(const int fd);

void *my_malloc(size_t len);

int do_exit(char *str, const char **tab, char **env);

char **copie_environemnt(const char **environ, int ac);

char **my_wordtab(const char *str, int b, char param);

#endif    /* !MYSH_H_ */
