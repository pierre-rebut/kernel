/*
** get_next_line.c for 42sh in /home/despla_s/Epitech/Systeme_Unix/PSU_2014_42sh/src/tools
**
** Made by despla_s
** Login   <despla_s@epitech.net>
**
** Started on  Wed May  6 14:38:15 2015 despla_s
** Last update Wed May 27 09:29:35 2015 dourch_m
*/

#include <stdlib.h>
#include "get_next_line.h"

char *my_cat_len(char *stock, const char buff[BUFFER_SIZE + 1],
                 int i, int *save) {
    char *tmp;
    int compt;
    int x;
    int y;

    compt = 0;
    x = -1;
    y = -1;
    if (stock != NULL)
        while (stock[compt] != 0)
            compt++;
    if ((tmp = malloc(compt + i + 1)) == NULL)
        return (NULL);
    while (++x < compt)
        tmp[x] = stock[x];
    while (++y < i)
        tmp[x + y] = buff[*save + y];
    tmp[x + y] = '\0';
    if (stock != NULL)
        free(stock);
    *save = *save + i + 1;
    return (tmp);
}

char *get_next_line(const int fd) {
    static char buff[BUFFER_SIZE + 1];
    static int size = 0;
    static int save = 0;
    int i;
    char *ret_line;

    ret_line = NULL;
    i = 0;
    while (42) {
        if (save >= size) {
            save = 0;
            if ((size = read(fd, buff, BUFFER_SIZE)) <= 0)
                return (ret_line);
            i = 0;
        }
        if (buff[save + i] == '\n')
            return (my_cat_len(ret_line, buff, i, &save));
        if (save + i == size)
            ret_line = my_cat_len(ret_line, buff, i, &save);
        i++;
    }
}
