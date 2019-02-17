//
// Created by rebut_p on 11/02/19.
//

#ifndef KERNEL_GETOPT_H
#define KERNEL_GETOPT_H

struct option
{
  const char *name;
  int has_arg;
  int *flag;
  int val;
};

#define no_argument             0
#define required_argument       1
#define optional_argument       2

int getopt(int nargc, char *const *nargv, const char *options);
int getopt_long(int, char *const *, const char *, const struct option *, int *);
int getopt_long_only(int, char *const *, const char *, const struct option *, int *);

extern int opterr, optind, optopt, optreset;
extern char *optarg;

#endif //KERNEL_GETOPT_H
