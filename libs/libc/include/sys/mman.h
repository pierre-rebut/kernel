//
// Created by rebut_p on 16/02/19.
//

#ifndef _MMAN_H_
#define _MMAN_H_

#include <kernel/sys/mman.h>

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);

int munmap(void *addr, size_t len);

int mprotect(void *addr, size_t len, int prot);

#endif /* !_MMAN_H_ */
