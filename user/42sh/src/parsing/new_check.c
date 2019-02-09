/*
** new_check.c for 42sh in /home/ganive_v/rendu/systeme_unix/PSU_2014_42sh/src/parsing
**
** Made by Vincent Ganivet
** Login   <ganive_v@epitech.net>
**
** Started on  Sat May 23 18:56:26 2015 Vincent Ganivet
** Last update Sun May 24 18:58:27 2015 Vincent Ganivet
*/

#include    "define.h"
#include    "functions.h"

int next_elem(char *str) {
    int i;

    i = 0;
    while ((str[i] == '>' || str[i] == '<') && str[i] != '\0')
        i++;
    return (find_char(str + i + 1, " ><\0") + i + 1);
}

int order_redir(int redir, int sec_redir) {
    if (redir == sec_redir || (redir == 3 && sec_redir == 2) ||
        (redir == 2 && sec_redir == 3))
        return (FAIL);
    if (redir < sec_redir)
        redir = sec_redir;
    return (SUCCESS);
}

int count_redir(const char *str) {
    int i;
    int nb;

    nb = 0;
    i = 0;
    while (str[i]) {
        if ((str[i] == '>' || str[i] == '<') && str[i] != '\0') {
            while ((str[i] == '>' || str[i] == '<') && str[i] != '\0')
                i++;
            nb++;
        }
        while (str[i] != '>' && str[i] != '<' && str[i] != '\0')
            i++;
    }
    return (nb);
}

int binar_first(char *expr, int redir, int pipe) {
    int sec_red;

    if (check_binary(expr) == FAIL)
        return (FAIL);
    if (expr[find_char(expr, "><\0")] == '\0')
        return (SUCCESS);
    if ((redir = check_r_and_f(expr + find_char(expr, "><\0"))) == FAIL)
        return (FAIL);
    if (expr[next_elem(expr + find_char(expr, "><\0")) + find_char(expr, "><\0")
        ] == '\0')
        return (redir);
    if ((sec_red = check_r_and_f(epur_str(expr + next_elem
            (expr + find_char(expr, "><\0"))
                                          + find_char(expr, "><\0")))) == FAIL)
        return (FAIL);
    if ((redir = order_redir(redir, sec_red)) == FAIL)
        return (FAIL);
    if (pipe != 1)
        return (redir);
    return (ERROR_REDIR);
}

int check_expr(char *expr, int redir, int pipe) {
    int sec_red;

    if ((sec_red = check_r_and_f(expr)) == FAIL)
        return (binar_first(expr, redir, pipe));
    if (pipe == 2)
        return (FAIL);
    if ((redir = order_redir(redir, sec_red)) == FAIL)
        return (FAIL);
    if (check_binary(expr + next_elem(expr)) == FAIL)
        return (check_expr(epur_str(expr + next_elem(expr)), sec_red, pipe++));
    if (expr[find_char(expr + next_elem(expr), "><") + next_elem(expr)] == '\0')
        return (redir);
    else if ((redir = check_r_and_f(expr + find_char(expr + next_elem(expr),
                                                     "><\0") + next_elem(expr)))
             == FAIL)
        return (FAIL);
    else if ((redir = order_redir(redir, sec_red)) == FAIL)
        return (FAIL);
    if (pipe != 1)
        return (redir);
    return (ERROR_REDIR);
}
