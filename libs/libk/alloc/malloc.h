/*
** malloc.h for malloc in /home/rebut_p/Programmation/PSU_2015_malloc
** 
** Made by Pierre Rebut
** Login   <rebut_p@epitech.net>
** 
** Started on  Tue Jan 26 12:18:03 2016 Pierre Rebut
** Last update Mon Feb  1 19:09:07 2016 Pierre Rebut
*/

#ifndef KERNEL_MALLOC_H
#define KERNEL_MALLOC_H

#define FREE   1
#define MALLOC 0

#include <stddef.h>

typedef struct __attribute__((packed))    s_header {
    int magic_nb;
    size_t size;
    struct s_header *next;
    struct s_header *prev;
    int is_free;
} t_header;

extern t_header *g_begin;
extern size_t g_pagesize;


void *create_header(t_header *, size_t len,
                    t_header *next,
                    t_header *prev);

#endif /* !KERNEL_MALLOC_H */
