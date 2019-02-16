//
// Created by rebut_p on 16/02/19.
//

#ifndef KERNEL_ERRNO_BASE_H
#define KERNEL_ERRNO_BASE_H

#define	EPERM		 1	/* Operation not permitted */
#define	ENOENT		 2	/* No such file or directory */
#define	ESRCH		 3	/* No such process */
#define	EINTR		 4	/* Invalid system call number */
#define	EIO		     5	/* I/O error */
#define	ENXIO		 6	/* No such device or address */
#define	E2BIG		 7	/* Argument list too long */
#define	ENOEXEC		 8	/* Exec format error */
#define	EBADF		 9	/* Bad file number */
#define	ECHILD		10	/* No child processes */
#define	EAGAIN		11	/* Try again */
#define	ENOMEM		12	/* Out of memory */
#define	EACCES		13	/* Permission denied */
#define	EFAULT		14	/* Bad address */
#define	ENOTBLK		15	/* Block device required */
#define	EBUSY		16	/* Device or resource busy */
#define	EEXIST		17	/* File exists */
#define	ENODEV		18	/* No such device */
#define	ENOTDIR		19	/* Not a directory */
#define	EISDIR		20	/* Is a directory */
#define	EINVAL		21	/* Invalid argument */
#define	EMFILE		22	/* Too many open files */
#define	ESPIPE		23	/* Illegal seek */
#define	EROFS		24	/* Read-only file system */
#define	EPIPE		25	/* Broken pipe */
#define	EDEADLK		26	/* Resource deadlock would occur */

#define	ENAME2LONG	27	/* File name too long */

#define	ENOTEMPTY	28	/* Directory not empty */
#define	ELOOP		29	/* Too many symbolic links encountered */

#endif //KERNEL_ERRNO_BASE_H
