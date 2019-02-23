//
// Created by rebut_p on 10/02/19.
//

#ifndef LIBVGA_H
#define LIBVGA_H

#include <stddef.h>
#include <system/console.h>

extern enum ConsoleMode currentVideoMode;

int switchVgaMode(enum ConsoleMode mode);

void setVgaFrameBuffer(const void *buffer);


#define VGA_VIDEO_HEIGHT 200
#define VGA_VIDEO_WIDTH 320

#define VGA_AC_INDEX        0x3C0
#define VGA_AC_WRITE        0x3C0
#define VGA_AC_READ        0x3C1
#define VGA_MISC_WRITE        0x3C2
#define VGA_SEQ_INDEX        0x3C4
#define VGA_SEQ_DATA        0x3C5
#define VGA_DAC_MASK        0x3C6
#define VGA_DAC_READ_INDEX    0x3C7
#define VGA_DAC_WRITE_INDEX    0x3C8
#define VGA_DAC_DATA        0x3C9
#define VGA_MISC_READ        0x3CC
#define VGA_GC_INDEX        0x3CE
#define VGA_GC_DATA        0x3CF

#define VGA_CRTC_INDEX        0x3D4
#define VGA_CRTC_DATA        0x3D5
#define VGA_INSTAT_READ    0x3DA

#define VGA_NUM_SEQ_REGS    5
#define VGA_NUM_CRTC_REGS    25
#define VGA_NUM_GC_REGS    9
#define VGA_NUM_AC_REGS    21

#endif                /* !LIBVGA_H */
