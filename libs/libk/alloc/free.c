/*
** free.h for malloc in /home/rebut_p/Programmation/PSU_2015_malloc
** 
** Made by Pierre Rebut
** Login   <rebut_p@epitech.net>
** 
** Started on  Tue Jan 26 13:42:46 2016 Pierre Rebut
** Last update Mon Feb  8 11:41:54 2016 Pierre Rebut
*/

#include <alloc.h>
#include <unistd.h>

#include "malloc.h"

void delete_all(t_header *tmp)
{
    if (tmp != NULL && tmp->next == NULL) {
        if (tmp->prev == NULL) {
            brk(g_begin);
            g_pagesize = 0;
            g_begin = NULL;
        }
    }
}

void free(const void *ptr)
{
    t_header *pos;
    t_header *tmp;

    if (ptr != NULL && g_begin != NULL) {
        pos = (void *) (ptr - sizeof(t_header));
        pos->is_free = FREE;
        if (pos->next != NULL && pos->next->is_free == FREE) {
            pos->size += pos->next->size + sizeof(t_header);
            if (pos->next->next != NULL)
                pos->next->next->prev = pos;
            pos->next = pos->next->next;
        }
        if (pos->prev != NULL && pos->prev->is_free == FREE) {
            tmp = pos->prev;
            tmp->size += pos->size + sizeof(t_header);
            if (pos->next != NULL)
                pos->next->prev = tmp;
            tmp->next = pos->next;
        }
        delete_all(g_begin);
    }
}
