//
// Created by rebut_p on 29/09/18.
//

#ifndef KERNEL_EPITA_UTILS_H
#define KERNEL_EPITA_UTILS_H

#define max(a, b) ((a) >= (b) ? (a) : (b))
#define min(a, b) ((a) <= (b) ? (a) : (b))

#define CLEAR_BIT(val, bit) __asm__("btr %1, %0" : "+g"(val) : "r"(bit))
#define SET_BIT(val, bit) __asm__("bts %1, %0" : "+g"(val) : "r"(bit))

static inline u32 alignUp(u32 val, u32 alignment) {
    if (!alignment)
        return val;
    --alignment;
    return (val + alignment) & ~alignment;
}

static inline u32 alignDown(u32 val, u32 alignment) {
    if (!alignment)
        return val;
    return val & ~(alignment - 1);
}


#endif //KERNEL_EPITA_UTILS_H
