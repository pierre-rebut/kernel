//
// Created by rebut_p on 29/09/18.
//

#ifndef KERNEL_EPITA_CPU_H
#define KERNEL_EPITA_CPU_H

static inline void sti() {
    asm volatile("sti\n");
}

static inline void cli() {
    asm volatile("cli\n");
}

static inline void hlt() {
    asm volatile("hlt\n");
}

#endif //KERNEL_EPITA_CPU_H
