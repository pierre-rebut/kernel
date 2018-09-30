//
// Created by rebut_p on 30/09/18.
//

#include <stdio.h>
#include "error.h"
#include "../cpu.h"

void kpanic(const char *msg) {
    cli();
    printf("##################\n##\tKERNEL PANIC\t##\n##################\n\nERROR: %s\n", msg);
    while (1)
        hlt();
}

void ASSERT(int test) {
    if (test)
        return;
    kpanic("assert");
}