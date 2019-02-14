//
// Created by rebut_p on 11/02/19.
//

#ifndef KERNEL_GETOPT_H
#define KERNEL_GETOPT_H

int getopt(int nargc, char *const *nargv, const char *ostr);

extern int opterr, optind, optopt, optreset;
extern char *optarg;

#endif //KERNEL_GETOPT_H
