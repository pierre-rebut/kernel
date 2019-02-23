//
// Created by rebut_p on 16/02/19.
//

#ifndef _GRAPHIC_H_
#define _GRAPHIC_H_

#include "kstd.h"

#include "stddef.h"
#include "string.h"
#include "stdio.h"

/* this file should really be cleaned :) */

/*
 * some characteristics about the display.
 */
#define GRAPHIC_WIDTH    320
#define GRAPHIC_HEIGHT    200
#define FB_SIZE        (GRAPHIC_WIDTH * GRAPHIC_HEIGHT)
#define PALETTE_SIZE    256

/*
 * image  structure. used  load_image, clear_image  and  draw_image to
 * manipulate images.
 */

struct image
{
    unsigned int width;
    unsigned int height;
    unsigned char **data;
};

/*
 * animation structure.
 *
 */

struct anim
{
    int nr_img;
    int current_img;
    unsigned long delay;
    unsigned long jiffies;
    struct image **imgs;
};


/*
 * a color is an index in the palette.
 */
typedef unsigned int color_t;

/*
 * some colors.
 */
enum colors
{
    BLACK = 0,
    WHITE = 255,
    RED = 249,
    GREEN = 250,
    YELLOW = 251,
    BLUE = 252,
    PURPLE = 253,
    AQUA = 254,
    ORANGE = 23
};

/*
 * this function switches to graphic mode.
 */
void switch_graphic(void);

/*
 * this function get back to text mode.
 */
void switch_text(void);

/*
 * call this function at the beginning of drawing a frame.
 */
void draw_begin(void);

/*
 * call this function when finished drawing. this is the function that
 * copy your buffered draw from off-screen buffer to framebuffer.
 */
void draw_end(void);

/*
 * clears the screen with given color.
 */
void draw_clear(color_t color);

/*
 * this function plot a pixel of given color at given position.
 */
void draw_pixel(unsigned int x, unsigned int y, color_t color);

/*
 * draw a line.
 */
void draw_line(unsigned int x1, unsigned int y1,
               unsigned int x2, unsigned int y2, color_t color);

/*
 * draw an empty rectangle.
 */
void draw_rect(unsigned int x1, unsigned int y1,
               unsigned int x2, unsigned int y2, color_t color);

/*
 * draw a solid rectangle.
 */
void draw_fillrect(unsigned int x1, unsigned int y1,
                   unsigned int x2, unsigned int y2,
                   color_t color, color_t interior);

/*
 * load a Windows BITMAP (BMP) from file.
 * the only supported files are 8 bits per pixels paletted.
 * the only supported palette is the default one (obtained with Paint).
 */
struct image *load_image(const char *path);

/*
 * destroy a loaded image.
 */
void clear_image(struct image *image);

/*
 * display a loaded image with transparency.
 */
void draw_image_alpha(struct image *image,
                      unsigned int x, unsigned int y, unsigned int alpha);

/*
 * display a loaded image.
 */
void draw_image(struct image *image, unsigned int x, unsigned int y);

/*
 * draw some text.
 */
void draw_text(const char *s,
               unsigned int x, unsigned int y, color_t fg, color_t bg);

/*
 * load an animation.
 * paths is string containing the images name separated by a space.
 * load_anim supports the same image formats as load_image.
 * delay is the displaying time of each image (in ticks).
 *
 * invocation example: load_anim("pic1 pic2 pic3 pic4 pic5", PIC_ANIM_DELAY);
 */
struct anim *load_anim(char *paths, int delay);

/*
 * draw an animation at coordinates (x, y)
 *
 * jiffies is the reference ticks counter which should
 * be incremented at every timer tick.
 */
void draw_anim(struct anim *anim, int x, int y, unsigned long jiffies);

/*
 * video blue screen.
 */
extern void (*blue_screen)(const char *message);

#endif                /* !_GRAPHIC_H_ */
