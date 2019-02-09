//
// Created by rebut_p on 10/02/19.
//

#ifndef KERNEL_TTY_H
#define KERNEL_TTY_H

void initTTY(const char *, const char **, const char **);
void ttyTaskLoop();
void createNewTTY();

#endif //KERNEL_TTY_H
