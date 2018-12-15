//
// Created by rebut_p on 14/12/18.
//

#include <cpu.h>

#include "elf.h"
#include "sys/physical-memory.h"
#include "task.h"

#include <stdio.h>
#include <string.h>

u32 loadBinary(struct PageDirectory *pd, const void *data, u32 size) {

    const Elf32_Ehdr *binHeader = data;
    if (memcmp(binHeader->e_ident, ELFMAG, SELFMAG) != 0) {
        kSerialPrintf("bin header: wrong magic code\n");
        return 0;
    }

    kSerialPrintf("elf: %d - %d - %d\n", binHeader->e_ehsize, binHeader->e_phentsize, binHeader->e_entry);
    const Elf32_Phdr *prgHeader = (Elf32_Phdr*)(data + binHeader->e_phoff);
    for (u32 i = 0; i < binHeader->e_phnum; i++) {
        if ((const void*)(prgHeader + i) >= data + size)
            return (0);

        if (prgHeader[i].p_memsz == 0)
            continue;

        if (prgHeader[i].p_type != PT_LOAD)
            continue;

        u32 vaddr = prgHeader[i].p_vaddr;
        u32 memsz = alignUp(prgHeader[i].p_memsz, PAGESIZE);

        kSerialPrintf("vaddr = %X, vaddr alignUp %X\n", prgHeader[i].p_paddr, vaddr);
        kSerialPrintf("memsz = %X, memsz alignUp %X\n", prgHeader[i].p_memsz, memsz);

        if (pagingAlloc(pd, (void *)vaddr, memsz, MEM_USER | MEM_WRITE))
            return 0;

        kSerialPrintf("Paging alloc ok\n");

        cli();
        pagingSwitchPageDirectory(pd);
        memcpy((void *) vaddr, data + prgHeader[i].p_offset, prgHeader[i].p_filesz);
        memset((void *) (vaddr + prgHeader[i].p_filesz), 0, prgHeader[i].p_memsz - prgHeader[i].p_filesz);
        pagingSwitchPageDirectory(currentTask->pageDirectory);
        sti();



    }

    return binHeader->e_entry;
}