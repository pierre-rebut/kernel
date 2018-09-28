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
        printf("user bin: wrong magic code\n");
        return -1;
    }

    printf("elf: %d - %d - %d\n", binHeader.e_ehsize, binHeader.e_phentsize, binHeader.e_entry);

    Elf32_Phdr prgHeader1;
    size = read(fd, &prgHeader1, binHeader.e_phentsize);
    if (size != binHeader.e_phentsize)
        return -1;

    printf("prgHeader1: %d - %d - %d - %d\n", prgHeader1.p_type, prgHeader1.p_filesz, prgHeader1.p_offset,
           prgHeader1.p_flags);
    printf("prgHeader1: %d - %d - %d - %d\n", prgHeader1.p_memsz, prgHeader1.p_vaddr, prgHeader1.p_paddr,
           prgHeader1.p_align);

    Elf32_Phdr prgHeader2;
    size = read(fd, &prgHeader2, binHeader.e_phentsize);
    if (size != binHeader.e_phentsize)
        return -1;

    printf("prgHeader2: %d - %d - %d - %d\n", prgHeader2.p_type, prgHeader2.p_filesz, prgHeader2.p_offset,
           prgHeader2.p_flags);
    printf("prgHeader2: %d - %d - %d - %d\n", prgHeader2.p_memsz, prgHeader2.p_vaddr, prgHeader2.p_paddr,
           prgHeader2.p_align);


    u32 pos = module->mod_end + prgHeader1.p_vaddr;
    loadPrg(&prgHeader1, fd, pos);
    memset((void *) (pos + prgHeader1.p_filesz), 0, prgHeader1.p_memsz - prgHeader1.p_filesz);

    char data[10];
    read(fd, data, 10);

    pos = module->mod_end + prgHeader2.p_vaddr + 22;
    loadPrg(&prgHeader2, fd, pos);
    memset((void *) (pos + prgHeader2.p_filesz), 0, prgHeader2.p_memsz - prgHeader2.p_filesz);

    createTask(module->mod_end + binHeader.e_entry, pos + prgHeader2.p_memsz);
    return 0;
}