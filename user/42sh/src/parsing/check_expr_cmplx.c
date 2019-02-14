/*
** check_expr_cmplx.c for 42sh in /home/jeanma_e/SH/PSU_2014_42sh/src/parsing
**
** Made by etienne Jeanmart
** Login   <jeanma_e@epitech.net>
**
** Started on  Fri May 15 14:40:46 2015 etienne Jeanmart
** Last update Sun May 24 19:01:46 2015 despla_s
*/

#include <stdlib.h>
#include <stdio.h>
#include <err.h>

#include "define.h"
#include "functions.h"

int find_char(char *str, char *c) {
    int i;

    i = 0;
    while (compare_chara(str[i], c) != SUCCESS && str[i] != 0)
        i++;
    return (i);
}

char *strdup_to_char(char *str, char *chara) {
    char *ret;
    int i;

    i = 0;
    while (compare_chara(str[i], chara) != SUCCESS && str[i] != '\0')
        i++;
    if ((ret = malloc(sizeof(char) * (i + 1))) == NULL)
        return (NULL);
    i = 0;
    while (compare_chara(str[i], chara) != SUCCESS && str[i] != '\0') {
        ret[i] = str[i];
        i++;
    }
    ret[i] = 0;
    return (epur_str(ret));
}

char *my_strndup(const char *str, int lim) {
    char *ret;
    int i;

    if ((ret = malloc(sizeof(char) * (lim + 1))) == NULL)
        return (NULL);
    i = 0;
    while (i < lim && str[i] != '\0') {
        ret[i] = str[i];
        i++;
    }
    ret[i] = 0;
    ret = epur_str(ret);
    return (ret);
}

int check_error(int redir, char *ex_cplx, int pipe) {
    if (redir == ERROR_NULL) {
        printf("Invalid null command.\n");
        return (FAIL);
    }
    if (pipe == 1 && (redir == 2 || redir == 3)) {
        printf("Ambiguous output redirect.\n");
        return (FAIL);
    }
    if ((redir == 4 && ex_cplx[find_char(ex_cplx, "|")] != '\0') ||
        ((redir == 2 || redir == 3) && pipe == 1)) {
        printf("Ambiguous output redirect.\n");
        return (FAIL);
    }
    if (redir == FAIL) {
        printf("Missing name for redirect.\n");
        return (FAIL);
    }
    return (SUCCESS);
}

int check_expr_cmplx(char *ex_cplx, int pipe) {
    char *expr;
    int redir;

    ex_cplx = epur_str(ex_cplx);
    expr = strdup_to_char(ex_cplx, "|");
    if (count_redir(expr) > 2)
        return (check_error(2, ex_cplx, 1));
    if (expr[0] == '\0')
        return (check_error(ERROR_NULL, ex_cplx, pipe));
    redir = check_expr(expr, 0, pipe);
    free(expr);
    if (redir == ERROR_REDIR) {
        printf("Ambiguous output redirect.\n");
        return (FAIL);
    }
    if (check_error(redir, ex_cplx, pipe) == FAIL)
        return (FAIL);
    if (ex_cplx[find_char(ex_cplx, "|")] != 0)
        return (check_expr_cmplx(ex_cplx + find_char(ex_cplx, "|") + 1, 1));
    return (SUCCESS);
}
