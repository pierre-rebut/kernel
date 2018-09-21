//
// Created by rebut_p on 22/09/18.
//

#include "idt.h"
#include "pic.h"

#include <stdio.h>

void interrupt_handler(struct idt_context *ctx) {
    printf("toto %d\n", ctx->int_no);
    if (ctx->int_no > 31)
        pic_eoi_master();
}