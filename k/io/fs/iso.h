//
// Created by rebut_p on 22/12/18.
//

#ifndef KERNEL_ISO_H
#define KERNEL_ISO_H

#include <k/types.h>

struct iso_9660_directory_entry {
    u8 descriptor_length;
    u8 extended_sectors;
    u32 first_sector_little;
    u32 first_sector_big;
    u32 length_little;
    u32 length_big;
    u8 year;
    u8 month;
    u8 mday;
    u8 hour;
    u8 minute;
    u8 second;
    u8 timezone;
    u8 flags;
    u8 unit_size;
    u8 interleave_gap;
    u16 volume_sequence_little;
    u16 volume_sequence_big;
    u8 ident_length;
    char ident[1];
} __attribute__((packed));

#define ISO_9660_EXTENT_FLAG_HIDDEN     1
#define ISO_9660_EXTENT_FLAG_DIRECTORY  2

struct iso_9660_time {
    char year[4];
    char month[2];
    char mday[2];
    char hour[2];
    char minute[2];
    char second[2];
    char subsec[2];
    char timezone;
} __attribute__((packed));

#define ISO_9660_VOLUME_TYPE_BOOT 0
#define ISO_9660_VOLUME_TYPE_PRIMARY 1
#define ISO_9660_VOLUME_TYPE_SUPPLEMENTARY 2
#define ISO_9660_VOLUME_TYPE_PARTITION 3
#define ISO_9660_VOLUME_TYPE_TERMINATOR 255

struct iso_9660_volume_descriptor {
    u8 type;
    char magic[5];
    char other[2];
    char system[32];
    char volume[32];
    char reserved1[8];
    u32 nsectors_little;
    u32 nsectors_big;
    char reserved2[32];
    u16 volume_set_size_little;
    u16 volume_set_size_big;
    u16 volume_sequence_number_little;
    u16 volume_sequence_number_big;
    u16 sector_size_little;
    u16 sector_size_big;
    u32 path_table_size_little;
    u32 path_table_size_big;
    u32 first_path_table_start_little;
    u32 second_path_table_start_little;
    u32 first_path_table_start_big;
    u32 second_path_table_start_big;
    struct iso_9660_directory_entry root;
    char volume_set[128];
    char publisher[128];
    char preparer[128];
    char application[128];
    char copyright_file[37];
    char abstract_file[37];
    char bibliography_file[37];
    struct iso_9660_time creation_time;
    struct iso_9660_time modify_time;
    struct iso_9660_time expire_time;
    struct iso_9660_time effective_time;
    char unknown[2];
} __attribute__((packed));

#endif //KERNEL_ISO_H
