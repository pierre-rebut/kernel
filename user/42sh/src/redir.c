/*
** redir.c for 42 in /home/rebut_p/Programmation/Projet-b2/sys_unix/42sh/src
**
** Made by Pierre REBUT
** Login   <rebut_p@epitech.net>
**
** Started on  Fri May 22 18:22:03 2015 Pierre REBUT
** Last update Sun May 24 19:08:09 2015 despla_s
*/

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <define.h>
#include <filestream.h>
#include <alloc.h>

#include "functions.h"

int exe_redir_in_double(t_cmd *lst)
{
    int fd;
    char *tmp;

    my_putfd("> ", 1);
    fd = open("/tmp/test", O_RDWR | O_CREAT | O_TRUNC, 0x180);
    if (fd < 0) {
        printf("42sh : %s\n", strerror(errno));
        return FAIL;
    }

    while ((tmp = getline(stdin)) != NULL && strcmp(tmp, lst->redir_in + 1) != 0) {
        my_putfd("> ", 1);
        my_putfd(tmp, fd);
        my_putfd("\n", fd);
        free(tmp);
    }
    seek(fd, SEEK_SET, 0);
    return (fd);
}

int exe_redir_in_simple(t_cmd *lst)
{
    int fd;

    fd = open(lst->redir_in, O_RDONLY, 0);
    if (fd == -1) {
        printf("42sh : %s\n", strerror(errno));
        return (-1);
    }
    return (fd);
}

int exe_redir_in(t_cmd *lst)
{
    if (lst->redir_in[0] == '<')
        return exe_redir_in_double(lst);

    return exe_redir_in_simple(lst);
}

int exe_redir_out(t_cmd *lst, int fd)
{
    if (lst->redir_out[0] == '>') {
        fd = open(lst->redir_out + 1, O_WRONLY | O_CREAT | O_APPEND, 0x180);
        if (fd == -1) {
            printf("42sh : %s\n", strerror(errno));
            return (-1);
        }
    } else {
        fd = open(lst->redir_out, O_WRONLY | O_CREAT | O_TRUNC, 0x180);
        if (fd == -1) {
            printf("42sh : %s\n", strerror(errno));
            return (-1);
        }
    }
    return (fd);
}
