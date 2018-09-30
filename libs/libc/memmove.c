//
// Created by rebut_p on 30/09/18.
//

#include <string.h>

static void memcpyr(void* dest, const void* src, size_t bytes)
{
    // Calculate starting addresses
    void* temp = dest+bytes-1;
    src += bytes-1;

    size_t dwords = bytes/4;
    bytes %= 4;

    __asm__ volatile("std\n"
	             "rep movsb\n"
	             "sub $3, %%edi\n"
                     "sub $3, %%esi\n"
		     "mov %%edx, %%ecx\n"
                     "rep movsl\n"
	             "cld" : "+S"(src), "+D"(temp), "+c"(bytes) : "d"(dwords) : "memory");
}


void *memmove(void *destination, const void *source, size_t size) {
    if (source == destination || size == 0) {
        return (destination);
    }

    const unsigned int memMax = ~((const unsigned int) 0) - (size - 1);
    if ((const unsigned int) source > memMax || (const unsigned int) destination > memMax) {
        return (destination);
    }

    if (source < destination) {
        memcpyr(destination, source, size);
    } else {
        memcpy(destination, source, size);
    }
    return (destination);
}