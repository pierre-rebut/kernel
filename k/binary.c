//
// Created by rebut_p on 28/09/18.
//

#include "binary.h"
#include "kfilesystem.h"
#include "elf.h"
#include "task.h"
#include "cpu.h"

#include <stdio.h>
#include <string.h>
#include <utils.h>

u32 loadBinary(struct PageDirectory *pd, const void *data, u32 size) {

    const Elf32_Ehdr *binHeader = data;
    if (memcmp(binHeader->e_ident, ELFMAG, SELFMAG) != 0) {
        printf("bin header: wrong magic code\n");
        return 0;
    }

    printf("elf: %d - %d - %d\n", binHeader->e_ehsize, binHeader->e_phentsize, binHeader->e_entry);
    const Elf32_Phdr *ph = data + binHeader->e_phoff;
    for (u32 i = 0; i < binHeader->e_phnum; i++) {
        if ((const void*)(ph+i) >= data + size)
            return (0);

        if (ph[i].p_memsz == 0)
            continue;

        printf("prgHeader %d: %d - %d - %d - %d\n", i, ph[i].p_type, ph[i].p_filesz, ph[i].p_offset, ph[i].p_flags);
        printf("prgHeader %d: %d - %d - %d - %d\n", i, ph[i].p_memsz, ph[i].p_vaddr, ph[i].p_paddr, ph[i].p_align);

        u32 vaddr = alignUp(ph[i].p_vaddr, PAGESIZE);
        u32 memsz = alignUp(ph[i].p_memsz, PAGESIZE);

        printf("vaddr = %X, vaddr alignDown %X\n", ph[i].p_paddr, vaddr);
        printf("memsz = %X, memsz alignDown %X\n", ph[i].p_memsz, memsz);

        if (paging_alloc(pd, (void *) vaddr, memsz, MEM_USER | MEM_WRITE))
            return 0;

        printf("paging alloc ok\n");

        cli();
        struct PageDirectory *tmp = currentPageDirectory;
        switchPaging(pd);
        printf("memcpy start\n");
        memcpy(vaddr, data + ph[i].p_offset, ph[i].p_filesz);
        printf("memcpy end\n");
        memset((void *) (vaddr + ph[i].p_filesz), 0, ph[i].p_memsz - ph[i].p_filesz);
        printf("memset end\n");
        switchPaging(tmp);
        sti();

        if (!(ph[i].p_flags & PF_W)) // Disallow writing after we filled the page, if the ELF file doesn't request write-access
            paging_setFlags(pd, (void*)vaddr, memsz, MEM_USER);
    }

    return binHeader->e_entry;
}