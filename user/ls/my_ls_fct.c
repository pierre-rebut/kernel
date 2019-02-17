/*
** my_ls_fct.c for my_ls in /home/rebut_p/Programmation/Projet/my_ls
** 
** Made by rebut_p
** Login   <rebut_p@epitech.net>
** 
** Started on  Tue Nov 25 15:34:12 2014 rebut_p
** Last update Sun Nov 30 15:25:33 2014 rebut_p
*/

#include "kstd.h"
#include <alloc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "include/my_ls.h"

int do_file(t_option *opt, char *file) {
    char *direc;
    int fd;

    if ((fd = open(file, O_RDONLY, 0)) == -1)
        return (0);
    close(fd);
    if (opt->ll == 1) {
        direc = my_directory2(file);
        format_ll2(direc, file);
    } else {
        printf("%s\n", file);
    }
    return (1);
}

char *my_directory(char *address, char *name) {
    int i;
    int o;
    char *direc;

    i = 0;
    o = 0;
    direc = malloc(sizeof(char) * (strlen(address) + strlen(name) + 2));
    if (direc == NULL)
        return (NULL);
    while (address[i] != '\0')
        direc[o++] = address[i++];
    i = 0;
    if (direc[o - 1] != '/' || direc[o] != '/')
        direc[o++] = '/';
    while (name[i] != '\0')
        direc[o++] = name[i++];
    direc[o] = '\0';
    return (direc);
}

int format_ll2(char *file, char *name) {
    struct stat sb;

    if (stat(file, &sb) == -1)
        puts("Error stat");

    printf("%u %u %u %u %u %s\n", sb.st_mode, sb.st_size, sb.st_atim, sb.st_ctim, sb.st_mtim, name);
    free(file);
    return (1);
}

int format_ll(char *entry, char *dir) {
    char *direc;

    direc = my_directory(dir, entry);
    format_ll2(direc, entry);
    return (0);
}
