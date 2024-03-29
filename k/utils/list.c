//
// Created by rebut_p on 16/12/18.
//

#include <list.h>

#include <system/allocator.h>

static struct ListElem *createElem(void *data, struct ListElem *n, struct ListElem *p)
{
    struct ListElem *elem = kmalloc(sizeof(struct ListElem), 0, "addListElem");
    if (!elem)
        return NULL;

    elem->next = n;
    elem->prev = p;
    elem->data = data;
    return elem;
}

void listReset(struct List *lst)
{
    lst->begin = NULL;
    lst->cur = NULL;
}

void listAddElem(struct List *lst, void *data)
{
    if (lst->begin == NULL) {
        lst->begin = createElem(data, NULL, NULL);
        lst->cur = lst->begin;
        return;
    }

    struct ListElem *new = createElem(data, lst->begin, NULL);
    lst->begin->prev = new;
    lst->begin = new;
}

void listDeleteElem(struct List *lst, void *data)
{
    struct ListElem *tmp = lst->begin;
    while (tmp != NULL) {
        if (tmp->data == data) {
            if (tmp->next)
                tmp->next->prev = tmp->prev;
            if (tmp->prev)
                tmp->prev->next = tmp->next;
            else
                lst->begin = tmp->next;

            if (lst->cur == tmp) {
                if (tmp->next)
                    lst->cur = tmp->next;
                else
                    lst->cur = lst->begin;
            }

            kfree(tmp);
            return;
        }
        tmp = tmp->next;
    }
}

u32 listCountElem(struct List *lst)
{
    u32 i = 0;
    struct ListElem *tmp = lst->begin;

    while (tmp != NULL) {
        tmp = tmp->next;
        i++;
    }
    return i;
}

void *listGetElem(struct List *lst, int (*fct)(void *, va_list), ...)
{
    if (!lst)
        return NULL;

    struct ListElem *elem = lst->begin;

    va_list ap;
    va_start(ap, fct);

    while (elem) {
        va_list ap2;
        va_copy(ap2, ap);
        int i = fct(elem->data, ap2);
        va_end(ap2);

        if (i > 0) {
            va_end(ap);
            return elem->data;
        }

        elem = elem->next;
    }

    va_end(ap);
    return NULL;
}

void *listGetNextElem(struct List *lst)
{
    if (!lst || lst->begin == NULL)
        return NULL;

    if (lst->cur->next == NULL)
        lst->cur = lst->begin;
    else
        lst->cur = lst->cur->next;

    return lst->cur->data;
}

void *listGetElemByIndex(struct List *lst, u32 index)
{
    if (!lst)
        return NULL;

    u32 i = 0;
    struct ListElem *elem = lst->begin;

    while (elem) {
        if (i == index)
            return elem->data;

        elem = elem->next;
        i++;
    }
    return NULL;
}