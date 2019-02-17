//
// Created by rebut_p on 15/02/19.
//

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <filestream.h>
#include <err.h>
#include <kstd.h>
#include <errno.h>

int lflag = 0;
int cflag = 0;
int wflag = 0;

typedef unsigned long count_t;  /* Counter type */

/* Current file counters: chars, words, lines */
count_t ccount;
count_t wcount;
count_t lcount;

/* Totals counters: chars, words, lines */
count_t total_ccount = 0;
count_t total_wcount = 0;
count_t total_lcount = 0;

/* Output counters for given file */
void report(char *file, count_t ccount, count_t wcount, count_t lcount) {
    if (lflag) {
        fprintf(stdout, "%6lu ", lcount);
    }
    if (wflag)
        fprintf(stdout, "%6lu ", wcount);
    if (cflag)
        fprintf(stdout, "%6lu ", ccount);

    if (file)
        fprintf(stdout, "%s", file);
    fprintf(stdout, "\n");
}

/* Return true if C is a valid word constituent */
static int isword(unsigned char c) {
    return isalpha(c);
}

/* Increase character and, if necessary, line counters */
#define COUNT(c)       \
           ccount++;        \
           if ((c) == '\n') \
             lcount++;

/* Get next word from the input stream. Return 0 on end
   of file or error condition. Return 1 otherwise. */
int getword(FILE *fp) {
    int c;

    if (feof(fp))
        return 0;

    while ((c = getc(fp)) != EOF) {
        if (isword(c)) {
            wcount++;
            break;
        }
        COUNT (c);
    }

    for (; c != EOF; c = getc(fp)) {
        COUNT (c);
        if (!isword(c))
            break;
    }

    return c != EOF;
}

static void usage(void) {
    err("usage: wc [-lcw?] [file ...]\n");
}

int main(int argc, char **argv) {
    int ch;
    char *progname = argv[0];

    while ((ch = getopt(argc, argv, "lcw?")) != -1)
        switch (ch) {
            case 'l':
                lflag = 1;    /* -b implies -n */
                break;
            case 'c':
                cflag = 1;    /* -e implies -v */
                break;
            case 'w':
                wflag = 1;
                break;
            case '?':
            default:
                usage();
        }
    argv += optind;

    if (lflag == cflag && cflag == wflag && wflag == 0)
        lflag = cflag = wflag = 1;

    if (*argv != NULL) {
        for (; *argv != NULL; argv++) {
            FILE *file = fopen(*argv, O_RDONLY, 0);

            if (file == NULL) {
                printf("%s: %s: %s\n", progname, *argv, strerror(ENOENT));
                continue;
            }
            ccount = wcount = lcount = 0;
            while (getword(file));
            fclose(file);

            report(*argv, ccount, wcount, lcount);
            total_ccount += ccount;
            total_wcount += wcount;
            total_lcount += lcount;
        }

        if (argc - optind > 1)
            report("total", total_ccount, total_wcount, total_lcount);
    } else {
        ccount = wcount = lcount = 0;
        while (getword(stdin));
        report(NULL, ccount, wcount, lcount);
    }

    return 0;
}