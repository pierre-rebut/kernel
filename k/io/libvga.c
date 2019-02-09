/*
* Copyright (c) LSE
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY LSE AS IS AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL LSE BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <k/kstd.h>
#include <compiler.h>
#include "libvga.h"
#include "io.h"

enum ConsoleMode currentVideoMode = ConsoleModeText;

/*
** Use to save the VGA plane 2, which contains the text font,
** when we switch into graphic mode.
*/
static unsigned char libvga_txt_mode_font[320 * 200];

/*
** Registers value for graphic mode.
*/
static unsigned char libvga_regs_320x200x256[] = {
        /* MISC */
        0x63,
        /* SEQ */
        0x03, 0x01, 0x0F, 0x00, 0x0E,
        /* CRTC */
        0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
        0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
        0xFF,
        /* GC */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
        0xFF,
        /* AC */
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x41, 0x00, 0x0F, 0x00, 0x00
};

/*
** Registers value for basic text mode.
*/
static unsigned char libvga_regs_80x25xtext[] = {
        /* MISC */
        0x67,
        /* SEQ */
        0x03, 0x00, 0x03, 0x00, 0x02,
        /* CRTC */
        0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F,
        0x00, 0x4F, 0x0D, 0x0E, 0x00, 0x00, 0x00, 0x50,
        0x9C, 0x0E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3,
        0xFF,
        /* GC */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
        0xFF,
        /* AC */
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
        0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
        0x0C, 0x00, 0x0F, 0x08, 0x00
};

/*
** Basic Windows BITMAP pallet.
** This palette is automatically loaded when switching to mode 13h
*/
static unsigned int libvga_default_palette[256] = {
        0x0, 0x800000, 0x8000, 0x808000,
        0x80, 0x800080, 0x8080, 0xc0c0c0,
        0xc0dcc0, 0xa6caf0, 0x402000, 0x602000,
        0x802000, 0xa02000, 0xc02000, 0xe02000,
        0x4000, 0x204000, 0x404000, 0x604000,
        0x804000, 0xa04000, 0xc04000, 0xe04000,
        0x6000, 0x206000, 0x406000, 0x606000,
        0x806000, 0xa06000, 0xc06000, 0xe06000,
        0x8000, 0x208000, 0x408000, 0x608000,
        0x808000, 0xa08000, 0xc08000, 0xe08000,
        0xa000, 0x20a000, 0x40a000, 0x60a000,
        0x80a000, 0xa0a000, 0xc0a000, 0xe0a000,
        0xc000, 0x20c000, 0x40c000, 0x60c000,
        0x80c000, 0xa0c000, 0xc0c000, 0xe0c000,
        0xe000, 0x20e000, 0x40e000, 0x60e000,
        0x80e000, 0xa0e000, 0xc0e000, 0xe0e000,
        0x40, 0x200040, 0x400040, 0x600040,
        0x800040, 0xa00040, 0xc00040, 0xe00040,
        0x2040, 0x202040, 0x402040, 0x602040,
        0x802040, 0xa02040, 0xc02040, 0xe02040,
        0x4040, 0x204040, 0x404040, 0x604040,
        0x804040, 0xa04040, 0xc04040, 0xe04040,
        0x6040, 0x206040, 0x406040, 0x606040,
        0x806040, 0xa06040, 0xc06040, 0xe06040,
        0x8040, 0x208040, 0x408040, 0x608040,
        0x808040, 0xa08040, 0xc08040, 0xe08040,
        0xa040, 0x20a040, 0x40a040, 0x60a040,
        0x80a040, 0xa0a040, 0xc0a040, 0xe0a040,
        0xc040, 0x20c040, 0x40c040, 0x60c040,
        0x80c040, 0xa0c040, 0xc0c040, 0xe0c040,
        0xe040, 0x20e040, 0x40e040, 0x60e040,
        0x80e040, 0xa0e040, 0xc0e040, 0xe0e040,
        0x80, 0x200080, 0x400080, 0x600080,
        0x800080, 0xa00080, 0xc00080, 0xe00080,
        0x2080, 0x202080, 0x402080, 0x602080,
        0x802080, 0xa02080, 0xc02080, 0xe02080,
        0x4080, 0x204080, 0x404080, 0x604080,
        0x804080, 0xa04080, 0xc04080, 0xe04080,
        0x6080, 0x206080, 0x406080, 0x606080,
        0x806080, 0xa06080, 0xc06080, 0xe06080,
        0x8080, 0x208080, 0x408080, 0x608080,
        0x808080, 0xa08080, 0xc08080, 0xe08080,
        0xa080, 0x20a080, 0x40a080, 0x60a080,
        0x80a080, 0xa0a080, 0xc0a080, 0xe0a080,
        0xc080, 0x20c080, 0x40c080, 0x60c080,
        0x80c080, 0xa0c080, 0xc0c080, 0xe0c080,
        0xe080, 0x20e080, 0x40e080, 0x60e080,
        0x80e080, 0xa0e080, 0xc0e080, 0xe0e080,
        0xc0, 0x2000c0, 0x4000c0, 0x6000c0,
        0x8000c0, 0xa000c0, 0xc000c0, 0xe000c0,
        0x20c0, 0x2020c0, 0x4020c0, 0x6020c0,
        0x8020c0, 0xa020c0, 0xc020c0, 0xe020c0,
        0x40c0, 0x2040c0, 0x4040c0, 0x6040c0,
        0x8040c0, 0xa040c0, 0xc040c0, 0xe040c0,
        0x60c0, 0x2060c0, 0x4060c0, 0x6060c0,
        0x8060c0, 0xa060c0, 0xc060c0, 0xe060c0,
        0x80c0, 0x2080c0, 0x4080c0, 0x6080c0,
        0x8080c0, 0xa080c0, 0xc080c0, 0xe080c0,
        0xa0c0, 0x20a0c0, 0x40a0c0, 0x60a0c0,
        0x80a0c0, 0xa0a0c0, 0xc0a0c0, 0xe0a0c0,
        0xc0c0, 0x20c0c0, 0x40c0c0, 0x60c0c0,
        0x80c0c0, 0xa0c0c0, 0xfffbf0, 0xa0a0a4,
        0x808080, 0xff0000, 0xff00, 0xffff00,
        0xff, 0xff00ff, 0xffff, 0xffffff
};

static void libvga_set_palette(unsigned int *new_palette, u32 size) {
    outb(VGA_DAC_WRITE_INDEX, 0);
    for (u32 i = 0; i < size; i++) {
        outb(VGA_DAC_DATA, ((new_palette[i] >> 16) >> 2) & 0xFF);
        outb(VGA_DAC_DATA, ((new_palette[i] >> 8) >> 2) & 0xFF);
        outb(VGA_DAC_DATA, ((new_palette[i]) >> 2) & 0xFF);
    }
}

static void libvga_write_regs(unsigned char *regs) {
    unsigned int i;
    unsigned int a;

    /* write the MISC register */
    outb(VGA_MISC_WRITE, *regs);
    regs++;

    /* write SEQ registers */
    for (i = 0; i < VGA_NUM_SEQ_REGS; i++) {
        outb(VGA_SEQ_INDEX, i);
        outb(VGA_SEQ_DATA, *regs);
        regs++;
    }

    /* write CRTC registers */
    outb(VGA_CRTC_INDEX, 0x03);
    a = inb(VGA_CRTC_DATA);
    outb(VGA_CRTC_DATA, a | 0x80);
    outb(VGA_CRTC_INDEX, 0x11);
    a = inb(VGA_CRTC_DATA);
    outb(VGA_CRTC_DATA, a & ~0x80);
    regs[0x03] |= 0x80;
    regs[0x11] &= ~0x80;
    for (i = 0; i < VGA_NUM_CRTC_REGS; i++) {
        outb(VGA_CRTC_INDEX, i);
        outb(VGA_CRTC_DATA, *regs);
        regs++;
    }

    /* write GC registers */
    for (i = 0; i < VGA_NUM_GC_REGS; i++) {
        outb(VGA_GC_INDEX, i);
        outb(VGA_GC_DATA, *regs);
        regs++;
    }

    /* write AC registers */
    a = inb(VGA_INSTAT_READ);
    for (i = 0; i < VGA_NUM_AC_REGS; i++) {
        outb(VGA_AC_INDEX, i);
        outb(VGA_AC_WRITE, *regs);
        regs++;
    }
    a = inb(VGA_INSTAT_READ);
    outb(VGA_AC_INDEX, 0x20);

    /* write the default palette to the DAC */
    outb(VGA_DAC_MASK, 0xFF);
    libvga_set_palette(libvga_default_palette, array_size(libvga_default_palette));
}

static char *libvga_get_framebuffer(void) {
    unsigned int mmap_select;

    outb(VGA_GC_INDEX, 6);
    mmap_select = inb(VGA_GC_DATA);
    mmap_select >>= 2;
    mmap_select &= 3;
    switch (mmap_select) {
        case 0:
        case 1:
            return (char *) 0xA0000;
        case 2:
            return (char *) 0xB0000;
        case 3:
            return (char *) 0xB8000;
    }
    return (char *) 0;
}

static void libvga_switch_mode13h(void) {
    libvga_write_regs(libvga_regs_320x200x256);

    // plane 2 is now map in the memory, save it
    char *vram = libvga_get_framebuffer();
    for (u32 i = 0; i < array_size(libvga_txt_mode_font); i++) {
        libvga_txt_mode_font[i] = vram[i];
        vram[i] = 0;
    }
}

static void libvga_switch_mode3h(void) {
    // restore the VGA plane 2 to the text font
    char *vram = libvga_get_framebuffer();
    for (u32 i = 0; i < array_size(libvga_txt_mode_font); i++)
        vram[i] = libvga_txt_mode_font[i];

    libvga_write_regs(libvga_regs_80x25xtext);
}

int switchVgaMode(enum ConsoleMode mode) {
    if (mode == currentVideoMode)
        return 0;

    switch (mode) {
        case ConsoleModeText:
            libvga_switch_mode3h();
            break;
        case ConsoleModeVideo:
            libvga_switch_mode13h();
            break;
        default:
            return -1;
    }

    currentVideoMode = mode;
    consoleGetActiveConsole()->mode = mode;
    return 0;
}

void setVgaFrameBuffer(const void *buffer) {
    char *vram = libvga_get_framebuffer();
    for (u32 i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vram[i] = ((char*)buffer)[i];
}
