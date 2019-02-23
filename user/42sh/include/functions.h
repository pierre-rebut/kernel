/*
** functions.h for 42sh in /home/despla_s/Epitech/Systeme_Unix/PSU_2014_42sh/include
**
** Made by despla_s
** Login   <despla_s@epitech.net>
**
** Started on  Wed May  6 14:38:41 2015 despla_s
** Last update Sun Jun  7 15:31:13 2015 dourch_m
*/

#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include "struct.h"
#include <kstd.h>

/*
** parsing/tools.c
*/

char *shift(char *str, int i);

int compare_chara(char c, const char *str);

/*
** redir.c
*/

int exe_redir_in(t_cmd *lst);

int exe_redir_out(t_cmd *lst, int fd);

/*
** parser.c
*/

int parser(char *cmd);

int exec_line(char *cmd, t_env *env, int *exit_val);

int check_tab(char **tab, int i, int value);

/*
** builtin
*/

int cd_call(int fd, t_cmd *cmd, t_env *env);

int env_call(int fd, t_cmd *cmd, t_env *env);

int kill_call(int fd, t_cmd *cmd, t_env *env);

int setenv_call(int fd, t_cmd *cmd, t_env *env);

int unsetenv_call(int fd, t_cmd *cmd, t_env *env);

int echo(int fd, t_cmd *cmd, t_env *env);

int sync_call(int fd, t_cmd *cmd, t_env *env);

char *cd_check_arg(char *path, t_env *env, int fd);

char *cd_tild(t_env *env);

char *cd_dash(t_env *env, int fd);

int modify_var(char *name, char *new_var, t_env *env);

int add_var(char *new_env_var, t_env *env);

int remove_var(char **new_env, int line_cpy, t_env *env);

int exec_built(t_cmd *lst, t_env *env);

/*
** exec.c
*/

char *get_prg(char *prg, t_env *env);

int exec_prg(struct ExceveInfo *execInfo, t_cmd *lst, t_env *env);

int exec_lst(t_cmd *lst, t_env *env);

/*
** create_liste.c
*/

t_cmd *init_list();

t_cmd *reversechain(t_cmd *lst);

t_cmd *create_list(char **cmd);


/*
** tools
*/

int my_putfd(char *str, int fd);

int check_nb(const char *str);

int check_pipe(t_cmd *lst);

/*
** str_to_wordtab.c
*/
char **str_parser_line(char *str, int i, int y);

char **my_wordtab(char *str, char param, int i, int y);

/*
** memory
*/

int free_list(t_cmd *lst);

int tab_len(char **tab);

int free_tab(char **tab);

/*
** tools.c
*/

char **create_env(char **tab);

char *my_epur_cmd(char *str, int i);

char *epur_str(char *str);

char *my_getenv(char *, char **);

int my_getenv_line(char *, char **);

/*
** parser
*/

int parser(char *);

int check_expr_cmplx(char *, int);

void put_error(char *);

int check_expr(char *, int, int);

int next_elem(char *);

char *my_strndup(const char *, int);

char *strdup_to_char(char *, char *);

int count_redir(const char *);

int check_binary(char *);

int check_r_and_f(char *);

int find_char(char *, char *);

#endif        /* !FUNCTIONS_H_ */
