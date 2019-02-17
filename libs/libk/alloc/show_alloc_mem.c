/*
** show_alloc_mem.c for showmem in /home/rebut_p/Programmation/PSU_2015_malloc
** 
** Made by Pierre Rebut
** Login   <rebut_p@epitech.net>
** 
** Started on  Mon Feb  1 15:02:59 2016 Pierre Rebut
** Last update Mon Feb  8 11:49:20 2016 dourch_m
*/

#include <stdio.h>
#include <unistd.h>

#include "malloc.h"

void show_alloc_mem() {
    t_header *tmp;

    tmp = g_begin;
    printf("break : %p\n", sbrk(0));
    while (tmp != NULL) {
        if (tmp->is_free == MALLOC) {
            printf("%p - %p : %u bytes\n", tmp, tmp + sizeof(t_header) + tmp->size, tmp->size + sizeof(t_header));
        }
        tmp = tmp->next;
    }
}

int getpagesize() {
    static int pageSize = -1;

    if (pageSize == -1) {
        pageSize = sysconf(_SC_PAGESIZE);
    }

    return pageSize;
}
