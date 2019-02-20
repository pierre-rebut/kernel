/*
** realloc.c for realloc in /home/dourches/Epitech/Tek2/PSU_2015_malloc
** 
** Made by dourch_m
** Login   <dourches@epitech.net>
** 
** Started on  Tue Jan 26 14:07:52 2016 dourch_m
** Last update Tue Feb  9 15:47:51 2016 Pierre Rebut
*/

#include <string.h>
#include <alloc.h>
#include <unistd.h>

#include "malloc.h"

static void *allocnext(t_header *ptr, size_t size, t_header *tmpptr)
{
    size_t tmp;
    t_header *new;

    tmp = ptr->size + ptr->next->size + sizeof(t_header) - size;
    if (tmp >= sizeof(t_header)) {
        new = create_header((void *) ptr + size + sizeof(t_header),
                            ptr->size + ptr->next->size + sizeof(t_header) - size,
                            tmpptr, ptr);
        new->is_free = FREE;
        if (tmpptr != NULL)
            tmpptr->prev = new;
        ptr->next = new;
    } else {
        if (tmpptr != NULL)
            tmpptr->prev = ptr;
        ptr->next = tmpptr;
    }
    ptr->next = tmpptr;
    ptr->size = size;
    ptr->is_free = MALLOC;
    return ((void *) ptr + sizeof(t_header));
}

static void *reduce_size(t_header *ptr, size_t size)
{
    t_header *new;
    t_header *link;
    size_t oldsize;

    oldsize = ptr->size;
    ptr->size = size;
    if (oldsize - size >= sizeof(t_header)) {
        link = ptr->next;
        new = create_header((void *) ptr + size + sizeof(t_header),
                            oldsize - size - sizeof(t_header), ptr->next, ptr);
        if (link != NULL)
            link->prev = new;
        ptr->next = new;
    }
    return ((void *) ptr + sizeof(t_header));
}

static void *allocnextnull(t_header *ptr, size_t size)
{
    size_t sizeloop;

    sizeloop = 0;
    while (g_pagesize <= size - ptr->size) {
        sizeloop++;
        g_pagesize += getpagesize();
    }
    if (sizeloop != 0)
        if (sbrk(getpagesize() * sizeloop) == NULL)
            return (NULL);
    g_pagesize -= size - ptr->size;
    ptr->size = size;
    return ((void *) ptr + sizeof(t_header));
}

static void *check_all(t_header *ptr, size_t size)
{
    void *tmp;

    if (ptr->size == size)
        return ((void *) ptr + sizeof(t_header));
    else if (ptr->size > size)
        return (reduce_size(ptr, size));
    else if (ptr->next != NULL && ptr->next->is_free == 1 &&
             ptr->next->size + ptr->size + sizeof(t_header) >= size)
        return (allocnext(ptr, size, ptr->next->next));
    else if (ptr->next == NULL)
        return (allocnextnull(ptr, size));
    else {
        tmp = malloc(size);
        if (tmp == NULL)
            return (NULL);
        memcpy(tmp, (void *) ptr + sizeof(t_header), ptr->size);
        free((void *) ptr + sizeof(t_header));
        return (tmp);
    }
}

void *realloc(void *ptr, size_t size)
{
    void *new;
    t_header *pos;

    if (ptr == NULL)
        return (malloc(size));
    if (size == 0) {
        free(ptr);
        return (NULL);
    }
    pos = ptr - sizeof(t_header);
    if (pos->magic_nb != 42)
        return (NULL);
    new = check_all(pos, size);
    return (new);
}
