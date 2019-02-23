//
// Created by rebut_p on 16/02/19.
//

#ifndef _SOUND_H
#define _SOUND_H

#include <kernel/sound.h>

#include "kstd.h"
#include "stddef.h"
#include "string.h"

int playsound(struct melody *melody, int repeat);

struct melody *load_sound(const char *path);

void clear_sound(struct melody *melody);

#endif                /* !_SOUND_H_ */
