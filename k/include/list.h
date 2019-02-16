//
// Created by rebut_p on 16/12/18.
//

#ifndef KERNEL_LIST_H
#define KERNEL_LIST_H

#include <stdarg.h>

struct ListElem {
    void *data;
    struct ListElem *next;
    struct ListElem *prev;
};

struct List {
    struct ListElem *begin;
    struct ListElem *cur;
};

#define CREATE_LIST() {0, 0}
void listReset(struct List *lst);
void listAddElem(struct List *lst, void *data);
void listDeleteElem(struct List *lst, void *data);

void *listGetElem(struct List *lst, int (*fct)(void *, va_list ap), ...);

void *listGetNextElem(struct List *lst);
u32 listCountElem(struct List *lst);

void *listGetElemByIndex(struct List *lst, u32 index);

#endif //KERNEL_LIST_H
