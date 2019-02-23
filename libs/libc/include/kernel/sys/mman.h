//
// Created by rebut_p on 23/02/19.
//

#ifndef _KERNEL_MMAN_H
#define _KERNEL_MMAN_H

#define PROT_READ       0x1             /* Page can be read.  */
#define PROT_WRITE      0x2             /* Page can be written.  */
#define PROT_EXEC       0x4             /* Page can be executed.  */
#define PROT_NONE       0x0             /* Page can not be accessed.  */
#define PROT_GROWSDOWN  0x01000000      /* Extend change to start of
                                           growsdown vma (mprotect only).  */
#define PROT_GROWSUP    0x02000000      /* Extend change to start of
                                           growsup vma (mprotect only).  */

#define MAP_SHARED      0x01
#define MAP_PRIVATE     0x02
#define MAP_ANONYMOUS   0x20

#define MAP_FAILED      ((void *) -1)

#endif //_KERNEL_MMAN_H
