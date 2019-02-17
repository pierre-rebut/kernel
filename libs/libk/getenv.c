//
// Created by rebut_p on 17/02/19.
//

#include <string.h>

char *getenv(const char *name) {
    if (name == NULL)
        return NULL;

    char **env = (char **) 0x1501000;
    u32 size = strlen(name);

    while (*env != NULL) {
        if (strncmp(name, *env, size) == 0)
            return (*env) + size + 1;

        env ++;
    }

    return NULL;
}