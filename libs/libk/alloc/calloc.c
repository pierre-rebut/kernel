/*
** calloc.c for calloc in /home/rebut_p/Programmation/PSU_2015_malloc
** 
** Made by Pierre Rebut
** Login   <rebut_p@epitech.net>
** 
** Started on  Wed Jan 27 16:18:28 2016 Pierre Rebut
** Last update Tue Feb  9 15:17:04 2016 Pierre Rebut
*/

#include <string.h>
#include <alloc.h>

void *calloc(size_t len, size_t u) {
    void *tmp;

    if (len == 0 || u == 0)
        return (NULL);
    tmp = malloc(len * u);
    if (tmp == NULL)
        return (NULL);
    memset(tmp, 0, len * u);
    return (tmp);
}
