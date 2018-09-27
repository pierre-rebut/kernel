//
// Created by rebut_p on 28/09/18.
//

#include "binary.h"
#include "kfilesystem.h"
#include "elf.h"
#include "userland.h"
#include "multiboot.h"
#include "gdt.h"

#include <stdio.h>
#include <string.h>

static u32 loadPrg(const Elf32_Phdr *prgHeader, int fd, u32 pos) {
    u32 limit = pos;

    ssize_t size = 0;
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

struct task {
    u32 esp;
    u32 dataSegment;
};

static struct task myTask = {0};

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
    //addUserlandEntry(USERCODE, pos, pos + prgHeader1.p_memsz);

    char data[10];
    read(fd, data, 10);

    pos = module->mod_end + prgHeader2.p_vaddr;
    loadPrg(&prgHeader2, fd, pos);
    memset((void *) (pos + prgHeader2.p_filesz), 0, prgHeader2.p_memsz - prgHeader2.p_filesz);
    //addUserlandEntry(USERDATA, pos, pos + prgHeader2.p_memsz);

    pos += prgHeader2.p_memsz;
    //addUserlandEntry(USERSTACK, pos, pos + 4096);
    //addUserlandEntry(USERHEAP, pos + 4096, pos + 4096);

    // set ds, fs, gs, ss
    u32 *stack = (u32 *) (pos + 4096);

    *(--stack) = 0x0000;  // ss
    *(--stack) = 0x6000;  // esp
    *(--stack) = 0x20202; // eflags = vm86 (bit17), interrupts (bit9), iopl=0

    *(--stack) = 0;    // cs
    *(--stack) = module->mod_end + binHeader.e_entry; // eip
    *(--stack) = 0;               // error code
    *(--stack) = 0;               // Interrupt nummer

// General purpose registers w/o esp
    *(--stack) = 0;            // eax. Used to give argc to user programs.
    *(--stack) = 0; // ecx. Used to give argv to user programs.
    *(--stack) = 0; //cpu_supports(CF_SYSENTEREXIT); // edx. Used to inform the user programm about the support for the SYSENTER/SYSEXIT instruction. TODO: Enable it. At the moment, there are problems with multiple tasks
    *(--stack) = 0;
    *(--stack) = 0;
    *(--stack) = 0;
    *(--stack) = 0;
    *(--stack) = 0;

    myTask.dataSegment = 0x23;
    myTask.esp = (u32) stack;

    // todo launch bin
    //asm volatile("int $50");
    return 0;
}

u32 task_switch() {
    switchTSS(myTask.esp, myTask.esp, myTask.dataSegment);
    return myTask.esp;
}