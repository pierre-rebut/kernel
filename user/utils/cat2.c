
static char const copyright[] =
        "@(#) Copyright (c) 1989, 1993\nThe Regents of the University of California.  All rights reserved.";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <err.h>
#include <filestream.h>
#include <kstd.h>
#include <syscallw.h>

static int bflag, eflag, nflag, sflag, tflag, vflag;
static int rval;
static const char *filename;

static void usage(void);

static void scanfiles(char *argv[], int cooked);

static void cook_cat(FILE *);

static void raw_cat(int);

/*
 * Memory strategy threshold, in pages: if physmem is larger than this,
 * use a large buffer.
 */
#define    PHYSPAGES_THRESHOLD (32 * 1024)

/* Maximum buffer size in bytes - do not allow it to grow larger than this. */
#define    BUFSIZE_MAX (2 * 1024 * 1024)

/*
 * Small (default) buffer size in bytes. It's inefficient for this to be
 * smaller than MAXPHYS.
 */
#define    BUFSIZE_SMALL (MAXPHYS)

int main(int argc, char *argv[]) {
    int ch;

    while ((ch = getopt(argc, argv, "benstv")) != -1)
        switch (ch) {
            case 'b':
                bflag = nflag = 1;    /* -b implies -n */
                break;
            case 'e':
                eflag = vflag = 1;    /* -e implies -v */
                break;
            case 'n':
                nflag = 1;
                break;
            case 's':
                sflag = 1;
                break;
            case 't':
                tflag = vflag = 1;    /* -t implies -v */
                break;;
            case 'v':
                vflag = 1;
                break;
            default:
                usage();
        }
    argv += optind;

    if (bflag || eflag || nflag || sflag || tflag || vflag)
        scanfiles(argv, 1);
    else
        scanfiles(argv, 0);
    return rval;
}

static void usage(void) {
    err("usage: cat [-benstuv] [file ...]\n\n%s\n", copyright);
}

static void scanfiles(char *argv[], int cooked) {
    int fd, i;
    char *path;

    i = 0;
    while ((path = argv[i]) != NULL || i == 0) {
        if (path == NULL || strcmp(path, "-") == 0) {
            filename = "stdin";
            fd = STDIN_FILENO;
        } else {
            filename = path;
            fd = open(path, O_RDONLY);
        }
        if (fd < 0) {
            printf("cat: %s: Aucun fichier ou dossier de ce type\n", path);
            rval = 1;
        } else if (cooked) {
            if (fd == STDIN_FILENO)
                cook_cat(stdin);
            else {
                FILE *fp = fdopen(fd);
                cook_cat(fp);
                fclose(fp);
            }
        } else {
            raw_cat(fd);
            if (fd != STDIN_FILENO)
                close(fd);
        }
        if (path == NULL)
            break;
        ++i;
    }
}

static void cook_cat(FILE *fp) {
    int ch, gobble, line, prev;

    /* Reset EOF condition on stdin. */
    if (fp == stdin && feof(stdin))
        clearerr(stdin);

    FILE *out = fdopen(1);
    if (out == NULL)
        return;

    line = gobble = 0;
    for (prev = '\n'; (ch = getc(fp)) != EOF; prev = ch) {
        if (prev == '\n') {
            if (sflag) {
                if (ch == '\n') {
                    if (gobble)
                        continue;
                    gobble = 1;
                } else
                    gobble = 0;
            }
            if (nflag) {
                if (!bflag || ch != '\n') {
                    fprintf(out, "%6d\t", ++line);
                } else if (eflag) {
                    fprintf(out, "%6s\t", "");
                }
            }
        }
        if (ch == '\n') {
            if (eflag && fputchar(out, '$') == EOF)
                break;
        } else if (ch == '\t') {
            if (tflag) {
                if (fputchar(out, '^') == EOF || fputchar(out, 'I') == EOF)
                    break;
                continue;
            }
        } else if (vflag) {
            if (!isascii(ch) && !isprint(ch)) {
                if (fputchar(out, 'M') == EOF || fputchar(out, '-') == EOF)
                    break;
            } else {
                if (fputchar(out, ch) == EOF)
                    break;
            }

            ch = -1;
            continue;
        }
        if (fputchar(out, ch) == EOF)
            break;
    }

    fflush(out);

    if (ferror(fp)) {
        warn("%s", filename);
        rval = 1;
        clearerr(fp);
    }
}

static void raw_cat(int rfd) {
    char buf[4097];
    ssize_t nr, nw;
    int off;

    while ((nr = read(rfd, buf, 4096)) > 0)
        for (off = 0; nr; nr -= nw, off += nw)
            if ((nw = write(1, buf + off, (size_t) nr)) < 0)
                err("stdout\n");

    if (nr < 0) {
        warn("%s\n", filename);
        rval = 1;
    }
}

