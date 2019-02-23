//
// Created by rebut_p on 16/02/19.
//

#ifndef _UIO_H_
#define _UIO_H_

#include <kernel/stddef.h>

#define UIO_MAXIOV 1024

struct iovec {
  void *iov_base;
  size_t iov_len;
};

ssize_t readv(int fd, const struct iovec *_iovec, int count);

ssize_t writev(int fd, const struct iovec *_iovec, int count);

#endif //_UIO_H_
