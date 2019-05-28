//
// Created by rebut_p on 27/02/19.
//

#include <unistd.h>
#include <stdio.h>

int main()
{
    printf("%u\n", fork());
    sleep(10);
    return 0;
}