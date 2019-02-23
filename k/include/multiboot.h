//
// Created by rebut_p on 16/12/18.
//

#ifndef MULTIBOOT_H_
#define MULTIBOOT_H_

#define MULTIBOOT_BOOTLOADER_MAGIC    0x2BADB002

typedef struct
{
    unsigned int magic;
    unsigned int flags;
    unsigned int checksum;
    unsigned int header_addr;
    unsigned int load_addr;
    unsigned int load_end_addr;
    unsigned int bss_end_addr;
    unsigned int entry_addr;
} multiboot_header_t;

typedef struct
{
    unsigned int tabsize;
    unsigned int strsize;
    unsigned int addr;
    unsigned int reserved;
} aout_symbol_table_t;

typedef struct
{
    unsigned int num;
    unsigned int size;
    unsigned int addr;
    unsigned int shndx;
} elf_section_header_table_t;

typedef struct
{
    unsigned int flags;
    unsigned int mem_lower;
    unsigned int mem_upper;
    unsigned int boot_device;
    unsigned int cmdline;
    unsigned int mods_count;
    unsigned int mods_addr;

    union
    {
        aout_symbol_table_t aout_sym;
        elf_section_header_table_t elf_sec;
    } u;

    unsigned int mmap_length;
    unsigned int mmap_addr;
} multiboot_info_t;

typedef struct
{
    unsigned int mod_start;
    unsigned int mod_end;
    unsigned int string;
    unsigned int reserved;
} module_t;

typedef struct
{
    unsigned int entrySize;
    unsigned long long regionAddr;
    unsigned long long regionSize;
    unsigned int type;
}  __attribute__((packed)) memory_map_t;

#endif                /* !MULTIBOOT_H_ */
