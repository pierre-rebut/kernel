//
// Created by rebut_p on 22/02/19.
//

#ifndef _SIGNAL_H
#define _SIGNAL_H

#include "stddef.h"
#include "sys/signum.h"

#define SIGSET_NWORDS (1024 / (8 * sizeof (unsigned long int)))

typedef struct {
  unsigned long int __val[SIGSET_NWORDS];
} sigset_t;

int kill(pid_t pid, int signal);

int pthread_sigmask(int how, const sigset_t *set, sigset_t *oldset);

int sigemptyset(sigset_t *set);

int sigfillset(sigset_t *set);

int sigaddset(sigset_t *set, int signum);

int sigdelset(sigset_t *set, int signum);

int sigismember(const sigset_t *set, int signum);

int sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oset);

#define SIG_BLOCK     0          /* Block signals.  */
#define SIG_UNBLOCK   1          /* Unblock signals.  */
#define SIG_SETMASK   2          /* Set the set of blocked signals.  */

#define MINSIGSTKSZ 2048

#endif //_SIGNAL_H
