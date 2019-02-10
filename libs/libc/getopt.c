//
// Created by rebut_p on 10/02/19.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int __db_getopt_reset;    /* global reset for VxWorks. */

int opterr = 1,        /* if error message should be printed */
        optind = 1,        /* index into parent argv vector */
        optopt,            /* character checked for validity */
        optreset;        /* reset getopt */
char *optarg;        /* argument associated with option */

#undef    BADCH
#define    BADCH    (int)'?'
#undef    BADARG
#define    BADARG    (int)':'
#undef    EMSG
#define    EMSG    ""

/*
 * getopt --
 *	Parse argc/argv argument vector.
 *
 * PUBLIC: #ifndef HAVE_GETOPT
 * PUBLIC: int getopt __P((int, char * const *, const char *));
 * PUBLIC: #endif
 */
int getopt(int nargc, char *const*nargv, const char *ostr) {
    static char *const*argv = NULL;
    static char *elem = NULL;
    static int pos = 0;

    if (argv != nargv) {
        argv = nargv;
        elem = argv[0];
        pos = 0;
    }

    if (elem )



    static char *progname;
    static char *place = EMSG;        /* option letter processing */
    char *oli;                /* option letter list index */

    /*
     * VxWorks needs to be able to repeatedly call getopt from multiple
     * programs within its global name space.
     */
    if (__db_getopt_reset) {
        __db_getopt_reset = 0;

        opterr = optind = 1;
        optopt = optreset = 0;
        optarg = NULL;
        progname = NULL;
        place = EMSG;
    }
    if (!progname) {
        progname = nargv;
        if ((progname = __db_rpath(*nargv)) == NULL)
            progname = *nargv;
        else
            ++progname;
    }

    if (optreset || !*place) {        /* update scanning pointer */
        optreset = 0;
        if (optind >= nargc || *(place = nargv[optind]) != '-') {
            place = EMSG;
            return (EOF);
        }
        if (place[1] && *++place == '-') {    /* found "--" */
            ++optind;
            place = EMSG;
            return (EOF);
        }
    }                    /* option letter okay? */
    if ((optopt = (int) *place++) == (int) ':' ||
        !(oli = strchr(ostr, optopt))) {
        /*
         * if the user didn't specify '-' as an option,
         * assume it means EOF.
         */
        if (optopt == (int) '-')
            return (EOF);
        if (!*place)
            ++optind;
        if (opterr && *ostr != ':')
            (void) fprintf(stderr, "%s: illegal option -- %c\n", progname, optopt);
        return (BADCH);
    }
    if (*++oli != ':') {            /* don't need argument */
        optarg = NULL;
        if (!*place)
            ++optind;
    } else {                    /* need an argument */
        if (*place)            /* no white space */
            optarg = place;
        else if (nargc <= ++optind) {    /* no arg */
            place = EMSG;
            if (*ostr == ':')
                return (BADARG);
            if (opterr)
                (void) fprintf(stderr,
                               "%s: option requires an argument -- %c\n",
                               progname, optopt);
            return (BADCH);
        } else                /* white space */
            optarg = nargv[optind];
        place = EMSG;
        ++optind;
    }
    return (optopt);            /* dump back option letter */
}
