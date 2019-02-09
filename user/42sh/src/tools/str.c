/*
** str.c for 42sh in /home/despla_s/Epitech/Systeme_Unix/PSU_2014_minishell2/src
**
** Made by despla_s
** Login   <despla_s@epitech.net>
**
** Started on  Wed May 13 15:57:14 2015 despla_s
** Last update Mon Jun  8 15:12:14 2015 dourch_m
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "functions.h"
#include "define.h"

int check_pipe(t_cmd *lst) {
    t_cmd *tmp;
    int i;

    i = 0;
    tmp = lst;
    while (tmp != NULL) {
        tmp = tmp->prev;
        i++;
    }
    if (i >= 50) {
        printf("Error: pipe.\n");
        return (-1);
    }
    return (0);
}

int check_nb(const char *str) {
    int i;

    i = 0;
    if ((str[0] == '-' || (str[0] < '0' || str[0] > '9')) && str[1] == '\0')
        return (-1);
    while (str[i]) {
        if ((str[i] < '0' || str[i] > '9') && i != 0)
            return (-1);
        else if (str[0] != '-' && (str[0] < '0' || str[0] > '9'))
            return (-1);
        i++;
    }
    return (0);
}

int my_putfd(char *str, int fd) {
    int len;

    len = strlen(str);
    if (fd != -1)
        return (write(fd, str, len));
    return (FAIL);
}
