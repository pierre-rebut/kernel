//
// Created by rebut_p on 16/11/18.
//

#ifndef KERNEL_EPITA_CPU_H
#define KERNEL_EPITA_CPU_H

static inline void cli() {
    asm volatile("cli\n\t");
}

static inline void sti() {
    asm volatile("sti\n\t");
}

static inline void hlt() {
    asm volatile("hlt\n\t");
}

#endif //KERNEL_EPITA_CPU_H
