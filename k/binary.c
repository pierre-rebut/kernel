//
// Created by rebut_p on 28/09/18.
//

#include "binary.h"
#include "elf.h"
#include "cpu.h"
#include "physical-memory.h"

#include <stdio.h>
#include <string.h>

u32 loadBinary(struct PageDirectory *pd, const void *data, u32 size) {

    const Elf32_Ehdr *binHeader = data;
    if (memcmp(binHeader->e_ident, ELFMAG, SELFMAG) != 0) {
        printf("bin header: wrong magic code\n");
        return 0;
    }

    printf("elf: %d - %d - %d\n", binHeader->e_ehsize, binHeader->e_phentsize, binHeader->e_entry);
    const Elf32_Phdr *prgHeader = (Elf32_Phdr*)(data + binHeader->e_phoff);
    for (u32 i = 0; i < binHeader->e_phnum; i++) {
        if ((const void*)(prgHeader + i) >= data + size)
            return (0);

        if (prgHeader[i].p_memsz == 0)
            continue;

        if (prgHeader[i].p_type != PT_LOAD)
            continue;

        u32 vaddr = alignUp(prgHeader[i].p_vaddr, PAGESIZE);
        u32 memsz = alignUp(prgHeader[i].p_memsz, PAGESIZE);

        printf("vaddr = %X, vaddr alignUp %X\n", prgHeader[i].p_paddr, vaddr);
        printf("memsz = %X, memsz alignUp %X\n", prgHeader[i].p_memsz, memsz);

        if (pagingAlloc(pd, (void *) vaddr, memsz, MEM_USER | MEM_WRITE))
            return 0;

        printf("Paging alloc ok\n");

        cli();
        struct PageDirectory *tmp = currentPageDirectory;
        switchPageDirectory(pd);
        memcpy((void *) vaddr, data + prgHeader[i].p_offset, prgHeader[i].p_filesz);
        memset((void *) (vaddr + prgHeader[i].p_filesz), 0, prgHeader[i].p_memsz - prgHeader[i].p_filesz);
        switchPageDirectory(tmp);
        sti();

        if (!(prgHeader[i].p_flags & PF_W))
            pagingSetFlags(pd, (void*)vaddr, memsz, MEM_USER);

    }

    return binHeader->e_entry;
}