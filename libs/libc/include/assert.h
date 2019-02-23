//
// Created by rebut_p on 16/02/19.
//

#ifndef _ASSERT_H
#define _ASSERT_H

#include "stdio.h"

#ifndef NDEBUG
#define assert(exp)                            \
     do                                    \
     {                                    \
       if (!(exp))                            \
       {                                \
         printf("%s, %d: assertion '%s' failed\n",            \
        __BASE_FILE__, __LINE__, exp);                \
         printf("System halted.\n");                    \
         while (1)                            \
       continue;                            \
       }                                \
     }                                    \
     while (0)
#endif

#endif                /* !_ASSERT_H */
