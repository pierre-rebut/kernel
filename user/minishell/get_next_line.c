/*
** get_next_line.c for get_next_line.c in /home/rebut_p/rendu/CPE_2014_get_next_line
**
** Made by rebut_p
** Login   <rebut_p@epitech.net>
**
** Started on  Sat Nov 22 10:44:35 2014 rebut_p
** Last update Mon Jan 19 11:48:56 2015 rebut_p
*/

#include    <fcntl.h>
#include    <stdlib.h>
#include    <unistd.h>
#include    <stdio.h>
#include    "mysh.h"

char *my_stcat(char *stack, const char buff[BUFFER_SIZE + 1], int len, int *save) {
    char *tmp;
    int cpt;
    int j;
    int i;

    cpt = 0;
    j = -1;
    i = -1;
    if (stack != NULL)
        while (stack[cpt] != '\0')
            cpt++;
    if ((tmp = malloc(sizeof(char) * (cpt + len + 1))) == NULL)
        return (NULL);
    while (++j < cpt)
        tmp[j] = stack[j];
    while (++i < len)
        tmp[j + i] = buff[*save + i];
    tmp[j + i] = '\0';
    if (stack != NULL)
        free(stack);
    *save = *save + len + 1;
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
        if (size <= save) {
            save = 0;
            if ((size = read(fd, buff, BUFFER_SIZE)) <= 0)
                return (ret_line);
            i = 0;
        }
        if (buff[save + i] == '\n')
            return (my_stcat(ret_line, buff, i, &save));
        if (save + i == size)
            ret_line = my_stcat(ret_line, buff, i, &save);
        i++;
    }
}
