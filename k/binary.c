//
// Created by rebut_p on 28/09/18.
//

#include "binary.h"
#include "kfilesystem.h"
#include "elf.h"
#include "task.h"

#include <stdio.h>
#include <string.h>

static u32 loadPrg(const Elf32_Phdr *prgHeader, int fd, u32 pos) {
    u32 limit = pos;

    size_t size = 0;
    while (size < prgHeader->p_filesz) {
        u8 data[500];
        ssize_t tmp = read(fd, data, 500);
        if (tmp == -1)
            break;

        memcpy((void *) limit, data, (size_t) tmp);
        limit += tmp;
        size += tmp;
    }
    return limit;
}

int loadBinary(const module_t *module, u32 cmdline) {
    int fd = open((const char *) (cmdline), O_RDONLY);
    if (fd < 0)
        return -1;

    Elf32_Ehdr binHeader;
    ssize_t size = read(fd, &binHeader, sizeof(Elf32_Ehdr));
    if (size != sizeof(Elf32_Ehdr))
        return -1;

    if (memcmp(binHeader.e_ident, ELFMAG, SELFMAG) != 0) {
        printf("bin header: wrong magic code\n");
        return -1;
    }

    printf("elf: %d - %d - %d\n", binHeader.e_ehsize, binHeader.e_phentsize, binHeader.e_entry);

    u32 pos = 0;

    for (u32 i = 0; i < binHeader.e_phnum; i++) {
        seek(fd, binHeader.e_phoff + (i * binHeader.e_phentsize), SEEK_SET);

        Elf32_Phdr prgHeader;
        size = read(fd, &prgHeader, binHeader.e_phentsize);
        if (size != binHeader.e_phentsize)
            return -1;

        printf("prgHeader %d: %d - %d - %d - %d\n", i, prgHeader.p_type, prgHeader.p_filesz, prgHeader.p_offset,
               prgHeader.p_flags);
        printf("prgHeader %d: %d - %d - %d - %d\n", i, prgHeader.p_memsz, prgHeader.p_vaddr, prgHeader.p_paddr,
               prgHeader.p_align);

        if (prgHeader.p_type != PT_LOAD)
            continue;

        seek(fd, prgHeader.p_offset, SEEK_SET);

        pos = prgHeader.p_vaddr;
        loadPrg(&prgHeader, fd, pos);
        memset((void *) (pos + prgHeader.p_filesz), 0, prgHeader.p_memsz - prgHeader.p_filesz);

        pos += prgHeader.p_memsz;
    }

    if (pos == 0)
        return -1;

    createTask(binHeader.e_entry, pos);
    return 0;
}