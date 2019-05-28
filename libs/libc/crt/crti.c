//
// Created by rebut_p on 15/02/19.
//

#include <filestream.h>

void _kinit()
{
    stdin = fdopen(0);
    stdout = fdopen(1);
    stderr = fdopen(2);
}