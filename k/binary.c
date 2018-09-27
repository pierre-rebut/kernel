//
// Created by rebut_p on 28/09/18.
//

#include "binary.h"
#include "kfilesystem.h"
#include "elf.h"
#include "userland.h"

#include <stdio.h>
#include <string.h>

static void loadPrg(u32 id, const Elf32_Phdr *prgHeader, const module_t *module, int fd, void *pos) {
    ssize_t size = 0;
    while (size < prgHeader->p_filesz) {
        u8 data[500];
        ssize_t tmp = read(fd, data, 500);
        if (tmp == -1)
            break;

        memcpy(pos, data, (size_t)tmp);
        pos += tmp;
        size += tmp;
    }

    addUserlandEntry(id, module->mod_end, (u32) pos);
}

int loadBinary(const module_t *module, u32 cmdline) {
    int fd = open((const char *) (cmdline + 1), O_RDONLY);
    if (fd < 0)
        return -1;

    Elf32_Ehdr binHeader;
    ssize_t size = read(fd, &binHeader, sizeof(Elf32_Ehdr));
    if (size != sizeof(Elf32_Ehdr))
        return -1;

    if (memcmp(binHeader.e_ident, ELFMAG, SELFMAG) != 0) {
        printf("user bin: wrong magic code\n");
        return -1;
    }

    printf("elf: %d - %d - %d\n", binHeader.e_ehsize, binHeader.e_phentsize, binHeader.e_entry);

    Elf32_Phdr prgHeader1;
    size = read(fd, &prgHeader1, binHeader.e_phentsize);
    if (size != binHeader.e_phentsize)
        return -1;

    printf("prgHeader1: %d - %d - %d - %d\n", prgHeader1.p_type, prgHeader1.p_filesz, prgHeader1.p_offset, prgHeader1.p_flags);

    Elf32_Phdr prgHeader2;
    size = read(fd, &prgHeader2, binHeader.e_phentsize);
    if (size != binHeader.e_phentsize)
        return -1;

    printf("prgHeader2: %d - %d - %d - %d\n", prgHeader2.p_type, prgHeader2.p_filesz, prgHeader2.p_offset, prgHeader2.p_flags);

    void *pos = (void*)module->mod_end;

    loadPrg(USERCODE, &prgHeader1, module, fd, pos);

    u8 data[10];
    read(fd, data, 10);

    loadPrg(USERDATA, &prgHeader2, module, fd, pos);

    return 0;
}