/*
** malloc2.c for malloc in /home/rebut_p/Programmation/PSU_2015_malloc
** 
** Made by Pierre Rebut
** Login   <rebut_p@epitech.net>
** 
** Started on  Wed Jan 27 11:41:51 2016 Pierre Rebut
** Last update Wed Feb 10 12:44:35 2016 Pierre Rebut
*/

#include <stdlib.h>
#include <unistd.h>

#include "malloc.h"

t_header *g_begin = NULL;
size_t g_pagesize = 0;

void *create_header(t_header *ptr, size_t len,
                    t_header *next, t_header *prev)
{
    if (ptr == NULL)
        return (NULL);
    ptr->magic_nb = 42;
    ptr->size = len;
    ptr->next = next;
    ptr->prev = prev;
    ptr->is_free = MALLOC;
    return ((void *) ptr);
}

void *malloc_new_pagesize(size_t len, t_header *header)
{
    size_t tmp;
    void *new;

    tmp = 0;
    while (g_pagesize <= len + sizeof(t_header)) {
        tmp++;
        g_pagesize += getpagesize();
    }
    if (tmp != 0)
        if (sbrk(getpagesize() * tmp) == NULL)
            return (NULL);
    g_pagesize -= (len + sizeof(t_header));
    new = create_header((void *) header + (header->size + sizeof(t_header)),
                        len, NULL, header);
    header->next = (t_header *) new;
    return (new + sizeof(t_header));
}

void *malloc_reallocsize(t_header *header, size_t len)
{
    size_t tmp;
    t_header *new;
    t_header *link;

    tmp = header->size;
    header->size = len;
    header->is_free = MALLOC;
    if (tmp - len >= sizeof(t_header)) {
        link = header->next;
        new = (t_header *) create_header((void *) header + len + sizeof(t_header),
                                         tmp - len - sizeof(t_header),
                                         link, header);
        new->is_free = FREE;
        if (link != NULL)
            link->prev = new;
        header->next = new;
    }
    return ((void *) header + sizeof(t_header));
}

void *malloc_page_size(size_t len, t_header *header)
{
    while (header->next != NULL) {
        if (header->magic_nb != 42)
            return (NULL);
        if (header->is_free == FREE && header->size >= len)
            return (malloc_reallocsize(header, len));
        header = header->next;
    }
    return (malloc_new_pagesize(len, header));
}

void *malloc(size_t len)
{
    void *new;

    if (len == 0)
        return (NULL);
    if (g_begin == NULL) {
        g_begin = sbrk(0);
        g_pagesize = 32 * (size_t) getpagesize();
        while (g_pagesize <= len + sizeof(t_header))
            g_pagesize += 10 * (size_t) getpagesize();
        if (sbrk(g_pagesize) == NULL)
            return (NULL);
        g_pagesize -= (len + sizeof(t_header));
        new = create_header(g_begin, len, NULL, NULL);
        return (new + sizeof(t_header));
    }
    new = malloc_page_size(len, g_begin);
    return (new);
}
