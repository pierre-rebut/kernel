//
// Created by rebut_p on 23/12/18.
//

#include <string.h>
#include "io/terminal.h"

int kputs(const char *s) {
    return writeStringTerminal(s, strlen(s));
}
