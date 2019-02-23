//
// Created by rebut_p on 16/02/19.
//

#include <sound.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>

#include "syscalls.h"

int playsound(struct melody *melody, int repeat)
{
    return syscall2(SYSCALL_PLAYSOUND, (u32) melody, (u32) repeat);
}

struct melody *load_sound(const char *path)
{
    struct melody *melody = NULL;
    int fd = -1;
    int nb = -1;
    int i = -1;
    char *magic = ".KSF";
    char buf[5];

    if ((fd = open(path, O_RDONLY, 0)) < 0)
        return NULL;

    buf[4] = 0;
    /* check the magic number */
    if ((read(fd, buf, 4) != 4) || (strcmp(magic, buf) != 0)) {
        close(fd);
        return NULL;
    }

    /* read the numbers of tones */
    if (read(fd, &nb, sizeof(int)) != sizeof(int)) {
        close(fd);
        return NULL;
    }

    /* allocate space to store the new melody */
    if (!(melody = malloc((nb + 1) * sizeof(struct melody)))) {
        close(fd);
        return NULL;
    }

    /* load the melody */
    for (i = 0; i < nb; i++) {
        if (read(fd, &melody[i].freq, sizeof(int)) != sizeof(int) ||
            read(fd, &melody[i].duration, sizeof(int)) != sizeof(int)) {
            close(fd);
            free(melody);
            return NULL;
        }
    }

    /* put a null tones to indicate end of melody */
    melody[nb].freq = 0;
    melody[nb].duration = (unsigned long) -1;

    close(fd);

    return (melody);
}

void clear_sound(struct melody *melody)
{
    free(melody);
}
