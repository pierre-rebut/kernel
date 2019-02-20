//
// Created by rebut_p on 17/02/19.
//

#include <err.h>
#include <errno.h>
#include <fts.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <utils.h>
#include <alloc.h>
#include <filestream.h>

#ifdef COLORLS
#include <ctype.h>
#include <termcap.h>
#include <signal.h>
#endif

#include "ls.h"
#include "extern.h"

static int printaname(const FTSENT *, u_long, u_long);

static void printlink(const FTSENT *);

static void printtime(time_t);

static int printtype(u_int);

static void printsize(size_t, off_t);

#ifdef COLORLS
static void	endcolor_termcap(int);
static void	endcolor_ansi(void);
static void	endcolor(int);
static int	colortype(mode_t);
#endif

static void aclmode(char *, const FTSENT *);

#define    IS_NOPRINT(p)    ((p)->fts_number == NO_PRINT)

#ifdef COLORLS
/* Most of these are taken from <sys/stat.h> */
typedef enum Colors {
    C_DIR,			/* directory */
    C_LNK,			/* symbolic link */
    C_SOCK,			/* socket */
    C_FIFO,			/* pipe */
    C_EXEC,			/* executable */
    C_BLK,			/* block special */
    C_CHR,			/* character special */
    C_SUID,			/* setuid executable */
    C_SGID,			/* setgid executable */
    C_WSDIR,		/* directory writeble to others, with sticky
				 * bit */
    C_WDIR,			/* directory writeble to others, without
				 * sticky bit */
    C_NUMCOLORS		/* just a place-holder */
} Colors;

static const char *defcolors = "exfxcxdxbxegedabagacad";

/* colors for file types */
static struct {
    int	num[2];
    int	bold;
} colors[C_NUMCOLORS];
#endif

static size_t padding_for_month[12];
static size_t month_max_size = 0;

void
printscol(const DISPLAY *dp)
{
    FTSENT *p;

    for (p = dp->list; p; p = p->fts_link) {
        if (IS_NOPRINT(p))
            continue;
        (void) printaname(p, dp->s_inode, dp->s_block);
        (void) fputchar(stdout, '\n');
    }
}

/*
 * print name in current style
 */
int
printname(const char *name)
{
    if (f_octal || f_octal_escape)
        return prn_octal(name);
    else if (f_nonprint)
        return prn_printable(name);
    else
        return fprintf(stdout, name);
}

static void
compute_abbreviated_month_size(void)
{
    int i;
    size_t width;
    size_t months_width[12];

    for (i = 0; i < 12; i++) {
        width = strlen(getMonthAbrev(i));
        if (width == (size_t) -1) {
            month_max_size = -1;
            return;
        }
        months_width[i] = width;
        if (width > month_max_size)
            month_max_size = width;
    }

    for (i = 0; i < 12; i++)
        padding_for_month[i] = month_max_size - months_width[i];
}

void printlong(const DISPLAY *dp)
{
    struct stat *sp;
    FTSENT *p;
    NAMES *np;
    char buf[20];
#ifdef COLORLS
    int color_printed = 0;
#endif

    if ((dp->list == NULL || dp->list->fts_level != FTS_ROOTLEVEL) &&
        (f_longform || f_size)) {
        (void) fprintf(stdout, "total %lu\n", howmany(dp->btotal, blocksize));
    }


    for (p = dp->list; p; p = p->fts_link) {
        if (IS_NOPRINT(p))
            continue;

        sp = p->fts_statp;
        if (f_inode)
            (void) fprintf(stdout, "%*ju ", dp->s_inode, (unsigned long int) sp->st_ino);
        if (f_size)
            (void) fprintf(stdout, "%*jd ", dp->s_block, howmany(sp->st_blocks, blocksize));
        strmode(sp->st_mode, buf);
        aclmode(buf, p);
        np = p->fts_pointer;
        (void) fprintf(stdout, "%s %*u %-*s  %-*s  ", buf, dp->s_nlink, (unsigned int) sp->st_nlink,
                      dp->s_user, np->user, dp->s_group, np->group);
        if (f_flags)
            (void) fprintf(stdout, "%-*s ", dp->s_flags, np->flags);
        if (f_label)
            (void) fprintf(stdout, "%-*s ", dp->s_label, np->label);

        printsize(dp->s_size, sp->st_size);
        if (f_accesstime)
            printtime(sp->st_atim);
        else if (f_statustime)
            printtime(sp->st_ctim);
        else
            printtime(sp->st_mtim);
#ifdef COLORLS
        if (f_color)
            color_printed = colortype(sp->st_mode);
#endif
        (void) printname(p->fts_name);
#ifdef COLORLS
        if (f_color && color_printed)
            endcolor(0);
#endif
        if (f_type)
            (void) printtype(sp->st_mode);
        if (S_ISLNK(sp->st_mode))
            printlink(p);
        (void) fputchar(stdout, '\n');
    }
}

void printstream(const DISPLAY *dp)
{
    FTSENT *p;
    int chcnt;

    for (p = dp->list, chcnt = 0; p; p = p->fts_link) {
        if (p->fts_number == NO_PRINT)
            continue;
        /* XXX strlen does not take octal escapes into account. */
        if (strlen(p->fts_name) + chcnt +
            (p->fts_link ? 2 : 0) >= (unsigned) termwidth) {
            fputchar(stdout, '\n');
            chcnt = 0;
        }
        chcnt += printaname(p, dp->s_inode, dp->s_block);
        if (p->fts_link) {
            fprintf(stdout, ", ");
            chcnt += 2;
        }
    }
    if (chcnt)
        fputchar(stdout, '\n');
}

void
printcol(const DISPLAY *dp)
{
    static FTSENT **array;
    static int lastentries = -1;
    FTSENT *p;
    FTSENT **narray;
    int base;
    int chcnt;
    int cnt;
    int col;
    int colwidth;
    int endcol;
    int num;
    int numcols;
    int numrows;
    int row;
    int tabwidth;

    if (f_notabs)
        tabwidth = 1;
    else
        tabwidth = 8;

    /*
     * Have to do random access in the linked list -- build a table
     * of pointers.
     */
    if (dp->entries > lastentries) {
        narray = realloc(array, dp->entries * sizeof(FTSENT *));
        if (narray == NULL) {
            warn(NULL);
            printscol(dp);
            return;
        }
        lastentries = dp->entries;
        array = narray;
    }
    for (p = dp->list, num = 0; p; p = p->fts_link)
        if (p->fts_number != NO_PRINT)
            array[num++] = p;

    colwidth = dp->maxlen;
    if (f_inode)
        colwidth += dp->s_inode + 1;
    if (f_size)
        colwidth += dp->s_block + 1;
    if (f_type)
        colwidth += 1;

    colwidth = (colwidth + tabwidth) & ~(tabwidth - 1);
    if (termwidth < 2 * colwidth) {
        printscol(dp);
        return;
    }
    numcols = termwidth / colwidth;
    numrows = num / numcols;
    if (num % numcols)
        ++numrows;

    if ((dp->list == NULL || dp->list->fts_level != FTS_ROOTLEVEL) &&
        (f_longform || f_size)) {
        (void) fprintf(stdout, "total %lu\n", howmany(dp->btotal, blocksize));
    }

    base = 0;
    for (row = 0; row < numrows; ++row) {
        endcol = colwidth;
        if (!f_sortacross)
            base = row;
        for (col = 0, chcnt = 0; col < numcols; ++col) {
            chcnt += printaname(array[base], dp->s_inode,
                                dp->s_block);
            if (f_sortacross)
                base++;
            else
                base += numrows;
            if (base >= num)
                break;
            while ((cnt = ((chcnt + tabwidth) & ~(tabwidth - 1)))
                   <= endcol) {
                if (f_sortacross && col + 1 >= numcols)
                    break;
                (void) fputchar(stdout, f_notabs ? ' ' : '\t');
                chcnt = cnt;
            }
            endcol += colwidth;
        }
        (void) fputchar(stdout, '\n');
    }
}

/*
 * print [inode] [size] name
 * return # of characters printed, no trailing characters.
 */
static int printaname(const FTSENT *p, u_long inodefield, u_long sizefield)
{
    struct stat *sp;
    int chcnt;
#ifdef COLORLS
    int color_printed = 0;
#endif

    sp = p->fts_statp;
    chcnt = 0;
    if (f_inode)
        chcnt += fprintf(stdout, "%*ju ",
                        (int) inodefield, (unsigned long int) sp->st_ino);
    if (f_size)
        chcnt += fprintf(stdout, "%*jd ",
                        (int) sizefield, howmany(sp->st_blocks, blocksize));
#ifdef COLORLS
    if (f_color)
        color_printed = colortype(sp->st_mode);
#endif
    chcnt += printname(p->fts_name);
#ifdef COLORLS
    if (f_color && color_printed)
        endcolor(0);
#endif
    if (f_type)
        chcnt += printtype(sp->st_mode);
    return (chcnt);
}

static size_t ls_strftime(char *str, size_t len, const char *fmt, const struct tm *tm)
{
    const char *posb;
    char nfmt[BUFSIZ];
    const char *format = fmt;
    size_t ret;

    if ((posb = strstr(fmt, "%b")) != NULL) {
        if (month_max_size == 0) {
            compute_abbreviated_month_size();
        }
        if (month_max_size > 0) {
            snprintf(nfmt, sizeof(nfmt), "%.*s%s%*s%s",
                     (int) (posb - fmt), fmt,
                     getMonthAbrev(tm->tm_mon),
                     (int) padding_for_month[tm->tm_mon],
                     "",
                     posb + 2);
            format = nfmt;
        }
    }
    ret = strftime(str, len, format, tm);
    return (ret);
}

static void printtime(time_t ftime)
{
    char longstring[80];
    static time_t now = 0;
    const char *format;
    static int d_first = -1;

    if (d_first < 0)
        d_first = true;
    if (now == 0)
        now = time(NULL);

#define    SIXMONTHS    ((365 / 2) * 86400)
    if (f_timeformat)  /* user specified format */
        format = f_timeformat;
    else if (f_sectime)
        /* mmm dd hh:mm:ss yyyy || dd mmm hh:mm:ss yyyy */
        format = d_first ? "%e %b %T %Y" : "%b %e %T %Y";
    else if (ftime + SIXMONTHS > now && ftime < now + SIXMONTHS)
        /* mmm dd hh:mm || dd mmm hh:mm */
        format = d_first ? "%e %b %R" : "%b %e %R";
    else
        /* mmm dd  yyyy || dd mmm  yyyy */
        format = d_first ? "%e %b  %Y" : "%b %e  %Y";
    ls_strftime(longstring, sizeof(longstring), format, localtime(&ftime));
    fprintf(stdout, "%s ", longstring);
}

static int printtype(u_int mode)
{

    if (f_slash) {
        if ((mode & S_IFMT) == S_IFDIR) {
            (void) fputchar(stdout, '/');
            return (1);
        }
        return (0);
    }

    switch (mode & S_IFMT) {
        case S_IFDIR:
            (void) fputchar(stdout, '/');
            return (1);
        case S_IFIFO:
            (void) fputchar(stdout, '|');
            return (1);
        case S_IFLNK:
            (void) fputchar(stdout, '@');
            return (1);
        case S_IFSOCK:
            (void) fputchar(stdout, '=');
            return (1);
        default:
            break;
    }
    if (mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
        (void) fputchar(stdout, '*');
        return (1);
    }
    return (0);
}

#ifdef COLORLS
static int
putch(int c)
{
    (void)fputchar(stdout, c);
    return 0;
}

static int
writech(int c)
{
    char tmp = (char)c;

    (void)write(STDOUT_FILENO, &tmp, 1);
    return 0;
}

static void
printcolor_termcap(Colors c)
{
    char *ansiseq;

    if (colors[c].bold)
        tputs(enter_bold, 1, putch);

    if (colors[c].num[0] != -1) {
        ansiseq = tgoto(ansi_fgcol, 0, colors[c].num[0]);
        if (ansiseq)
            tputs(ansiseq, 1, putch);
    }
    if (colors[c].num[1] != -1) {
        ansiseq = tgoto(ansi_bgcol, 0, colors[c].num[1]);
        if (ansiseq)
            tputs(ansiseq, 1, putch);
    }
}

static void
printcolor_ansi(Colors c)
{

    fprintf(stdout, "\033[");

    if (colors[c].bold)
        fprintf(stdout, "1");
    if (colors[c].num[0] != -1)
        fprintf(stdout, ";3%d", colors[c].num[0]);
    if (colors[c].num[1] != -1)
        fprintf(stdout, ";4%d", colors[c].num[1]);
    fprintf(stdout, "m");
}

static void
printcolor(Colors c)
{

    if (explicitansi)
        printcolor_ansi(c);
    else
        printcolor_termcap(c);
}

static void
endcolor_termcap(int sig)
{

    tputs(ansi_coloff, 1, sig ? writech : putch);
    tputs(attrs_off, 1, sig ? writech : putch);
}

static void
endcolor_ansi(void)
{

    fprintf(stdout, "\33[m");
}

static void
endcolor(int sig)
{

    if (explicitansi)
        endcolor_ansi();
    else
        endcolor_termcap(sig);
}

static int
colortype(mode_t mode)
{
    switch (mode & S_IFMT) {
    case S_IFDIR:
        if (mode & S_IWOTH)
            if (mode & S_ISTXT)
                printcolor(C_WSDIR);
            else
                printcolor(C_WDIR);
        else
            printcolor(C_DIR);
        return (1);
    case S_IFLNK:
        printcolor(C_LNK);
        return (1);
    case S_IFSOCK:
        printcolor(C_SOCK);
        return (1);
    case S_IFIFO:
        printcolor(C_FIFO);
        return (1);
    case S_IFBLK:
        printcolor(C_BLK);
        return (1);
    case S_IFCHR:
        printcolor(C_CHR);
        return (1);
    default:;
    }
    if (mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
        if (mode & S_ISUID)
            printcolor(C_SUID);
        else if (mode & S_ISGID)
            printcolor(C_SGID);
        else
            printcolor(C_EXEC);
        return (1);
    }
    return (0);
}

void
parsecolors(const char *cs)
{
    int i;
    int j;
    size_t len;
    char c[2];
    short legacy_warn = 0;

    if (cs == NULL)
        cs = "";	/* LSCOLORS not set */
    len = strlen(cs);
    for (i = 0; i < (int)C_NUMCOLORS; i++) {
        colors[i].bold = 0;

        if (len <= 2 * (size_t)i) {
            c[0] = defcolors[2 * i];
            c[1] = defcolors[2 * i + 1];
        } else {
            c[0] = cs[2 * i];
            c[1] = cs[2 * i + 1];
        }
        for (j = 0; j < 2; j++) {
            /* Legacy colours used 0-7 */
            if (c[j] >= '0' && c[j] <= '7') {
                colors[i].num[j] = c[j] - '0';
                if (!legacy_warn) {
                    warnx("LSCOLORS should use "
                        "characters a-h instead of 0-9 ("
                        "see the manual page)");
                }
                legacy_warn = 1;
            } else if (c[j] >= 'a' && c[j] <= 'h')
                colors[i].num[j] = c[j] - 'a';
            else if (c[j] >= 'A' && c[j] <= 'H') {
                colors[i].num[j] = c[j] - 'A';
                colors[i].bold = 1;
            } else if (tolower((unsigned char)c[j]) == 'x')
                colors[i].num[j] = -1;
            else {
                warnx("invalid character '%c' in LSCOLORS"
                    " env var", c[j]);
                colors[i].num[j] = -1;
            }
        }
    }
}

void
colorquit(int sig)
{
    endcolor(sig);

    (void)signal(sig, SIG_DFL);
    (void)kill(getpid(), sig);
}

#endif /* COLORLS */

static void printlink(const FTSENT *p)
{
    // int lnklen;
    char name[MAXPATHLEN + 1];
    // char path[MAXPATHLEN + 1];

    if (p->fts_level == FTS_ROOTLEVEL)
        (void) snprintf(name, sizeof(name), "%s", p->fts_name);
    else
        (void) snprintf(name, sizeof(name),
                        "%s/%s", p->fts_parent->fts_accpath, p->fts_name);
    /*if ((lnklen = readlink(name, path, sizeof(path) - 1)) == -1) {
        (void)ffprintf(stdout, stderr, "\nls: %s: %s\n", name, strerror(errno));
        return;
    }
    path[lnklen] = '\0';
    (void)fprintf(stdout, " -> ");
    (void)printname(path);*/
}

static void printsize(size_t width, off_t bytes)
{

    if (f_humanval) {
        /*
         * Reserve one space before the size and allocate room for
         * the trailing '\0'.
         */
        char buf[HUMANVALSTR_LEN - 1 + 1];

        humanize_number(buf, sizeof(buf), (long long) bytes, "", HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);
        (void) fprintf(stdout, "%*s ", (u_int) width, buf);
    } else if (f_thousands) {        /* with commas */
        /* This format assignment needed to work round gcc bug. */
        const char *format = "%*j'd ";
        (void) fprintf(stdout, format, (u_int) width, bytes);
    } else {
        (void) fprintf(stdout, "%*d ", (u_int) width, bytes);
    }
}

/*
 * Add a + after the standard rwxrwxrwx mode if the file has an
 * ACL. strmode() reserves space at the end of the string.
 */
static void aclmode(char *buf, const FTSENT *p)
{
    (void) buf;
    (void) p;
    /*char name[MAXPATHLEN + 1];
    int ret, trivial;
    static dev_t previous_dev = NODEV;
    static int supports_acls = -1;
    static int type = ACL_TYPE_ACCESS;
    acl_t facl;

    if (S_ISCHR(p->fts_statp->st_mode) || S_ISBLK(p->fts_statp->st_mode) ||
        S_ISWHT(p->fts_statp->st_mode))
        return;

    if (previous_dev == p->fts_statp->st_dev && supports_acls == 0)
        return;

    if (p->fts_level == FTS_ROOTLEVEL)
        snprintf(name, sizeof(name), "%s", p->fts_name);
    else
        snprintf(name, sizeof(name), "%s/%s",
            p->fts_parent->fts_accpath, p->fts_name);

    if (previous_dev != p->fts_statp->st_dev) {
        previous_dev = p->fts_statp->st_dev;
        supports_acls = 0;

        ret = lpathconf(name, _PC_ACL_NFS4);
        if (ret > 0) {
            type = ACL_TYPE_NFS4;
            supports_acls = 1;
        } else if (ret < 0 && errno != EINVAL) {
            warn("%s", name);
            return;
        }
        if (supports_acls == 0) {
            ret = lpathconf(name, _PC_ACL_EXTENDED);
            if (ret > 0) {
                type = ACL_TYPE_ACCESS;
                supports_acls = 1;
            } else if (ret < 0 && errno != EINVAL) {
                warn("%s", name);
                return;
            }
        }
    }
    if (supports_acls == 0)
        return;
    facl = acl_get_link_np(name, type);
    if (facl == NULL) {
        warn("%s", name);
        return;
    }
    if (acl_is_trivial_np(facl, &trivial)) {
        acl_free(facl);
        warn("%s", name);
        return;
    }
    if (!trivial)
        buf[10] = '+';
    acl_free(facl);*/
}