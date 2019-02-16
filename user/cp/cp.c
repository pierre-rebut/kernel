//
// Created by rebut_p on 15/02/19.
//

#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <types.h>
#include <err.h>
#include <string.h>
#include <stdio.h>

#include "extern.h"

#define	STRIP_TRAILING_SLASH(p) {					\
	while ((p).p_end > (p).p_path + 1 && (p).p_end[-1] == '/')	\
	*--(p).p_end = 0;						\
}

static char emptystring[] = "";

PATH_T to = { to.p_path, emptystring, "" };

int fflag, iflag, lflag, nflag, pflag, sflag, vflag;
static int Rflag, rflag;

enum op { FILE_TO_FILE, FILE_TO_DIR, DIR_TO_DNE };

static int copy(char *[], enum op, int);

int main(int argc, char *argv[])
{
    struct stat to_stat, tmp_stat;
    enum op type;
    int Hflag, Lflag, ch, r, fts_options = 0, have_trailing_slash;
    char *target;

    Hflag = Lflag = 0;
    while ((ch = getopt(argc, argv, "HLPRafilnprsv")) != -1)
        switch (ch) {
            case 'H':
                Hflag = 1;
                Lflag = 0;
                break;
            case 'L':
                Lflag = 1;
                Hflag = 0;
                break;
            case 'P':
                Hflag = Lflag = 0;
                break;
            case 'R':
                Rflag = 1;
                break;
            case 'a':
                pflag = 1;
                Rflag = 1;
                Hflag = Lflag = 0;
                break;
            case 'f':
                fflag = 1;
                iflag = nflag = 0;
                break;
            case 'i':
                iflag = 1;
                fflag = nflag = 0;
                break;
            case 'l':
                lflag = 1;
                break;
            case 'n':
                nflag = 1;
                fflag = iflag = 0;
                break;
            case 'p':
                pflag = 1;
                break;
            case 'r':
                rflag = Lflag = 1;
                Hflag = 0;
                break;
            case 's':
                sflag = 1;
                break;
            case 'v':
                vflag = 1;
                break;
            default:
                usage();
                break;
        }
    argc -= optind;
    argv += optind;

    if (argc < 2)
        usage();

    if (Rflag && rflag)
        err("the -R and -r options may not be specified together");
    if (lflag && sflag)
        err("the -l and -s options may not be specified together");
    if (rflag)
        Rflag = 1;
    if (Rflag) {
        if (Hflag)
            fts_options |= FTS_COMFOLLOW;
        if (Lflag) {
            fts_options &= ~FTS_PHYSICAL;
            fts_options |= FTS_LOGICAL;
        }
    } else {
        fts_options &= ~FTS_PHYSICAL;
        fts_options |= FTS_LOGICAL | FTS_COMFOLLOW;
    }

    /* Save the target base in "to". */
    target = argv[--argc];
    if (strlcpy(to.p_path, target, sizeof(to.p_path)) >= sizeof(to.p_path))
        err("%s: name too long", target);
    to.p_end = to.p_path + strlen(to.p_path);
    if (to.p_path == to.p_end) {
        *to.p_end++ = '.';
        *to.p_end = 0;
    }
    have_trailing_slash = (to.p_end[-1] == '/');
    if (have_trailing_slash)
    STRIP_TRAILING_SLASH(to);
    to.target_end = to.p_end;

    /* Set end of argument list for fts(3). */
    argv[argc] = NULL;

    /*
     * Cp has two distinct cases:
     *
     * cp [-R] source target
     * cp [-R] source1 ... sourceN directory
     *
     * In both cases, source can be either a file or a directory.
     *
     * In (1), the target becomes a copy of the source. That is, if the
     * source is a file, the target will be a file, and likewise for
     * directories.
     *
     * In (2), the real target is not directory, but "directory/source".
     */
    r = stat(to.p_path, &to_stat);
    if (r == -1 && errno != ENOENT)
        err("%s", to.p_path);
    if (r == -1 || !S_ISDIR(to_stat.st_mode)) {
        /*
         * Case (1).  Target is not a directory.
         */
        if (argc > 1)
            err("%s is not a directory", to.p_path);

        /*
         * Need to detect the case:
         *	cp -R dir foo
         * Where dir is a directory and foo does not exist, where
         * we want pathname concatenations turned on but not for
         * the initial mkdir().
         */
        if (r == -1) {
            if (Rflag && (Lflag || Hflag))
                stat(*argv, &tmp_stat);
            else
                stat(*argv, &tmp_stat);

            if (S_ISDIR(tmp_stat.st_mode) && Rflag)
                type = DIR_TO_DNE;
            else
                type = FILE_TO_FILE;
        } else
            type = FILE_TO_FILE;

        if (have_trailing_slash && type == FILE_TO_FILE) {
            if (r == -1) {
                err("directory %s does not exist",
                     to.p_path);
            } else
                err("%s is not a directory", to.p_path);
        }
    } else
        /*
         * Case (2).  Target is a directory.
         */
        type = FILE_TO_DIR;

    return copy(argv, type, fts_options);
}

static int copy(char *argv[], enum op type, int fts_options)
{
    struct stat to_stat;
    FTS *ftsp;
    FTSENT *curr;
    int base = 0, dne, badcp, rval;
    size_t nlen;
    char *p, *target_mid;
    mode_t mask = 0, mode = 0;

    /*
     * Keep an inverted copy of the umask, for use in correcting
     * permissions on created directories when not using -p.
     */
    //mask = ~umask(0777);
    //umask(~mask);

    if ((ftsp = fts_open(argv, fts_options, NULL)) == NULL)
        err("fts_open");
    for (badcp = rval = 0; (curr = fts_read(ftsp)) != NULL; badcp = 0) {
        switch (curr->fts_info) {
            case FTS_NS:
            case FTS_DNR:
            case FTS_ERR:
                warn("%s: %s",
                      curr->fts_path, strerror(curr->fts_errno));
                badcp = rval = 1;
                continue;
            case FTS_DC:			/* Warn, continue. */
                warn("%s: directory causes a cycle", curr->fts_path);
                badcp = rval = 1;
                continue;
            default:
                ;
        }

        /*
         * If we are in case (2) or (3) above, we need to append the
         * source name to the target name.
         */
        if (type != FILE_TO_FILE) {
            /*
             * Need to remember the roots of traversals to create
             * correct pathnames.  If there's a directory being
             * copied to a non-existent directory, e.g.
             *	cp -R a/dir noexist
             * the resulting path name should be noexist/foo, not
             * noexist/dir/foo (where foo is a file in dir), which
             * is the case where the target exists.
             *
             * Also, check for "..".  This is for correct path
             * concatenation for paths ending in "..", e.g.
             *	cp -R .. /tmp
             * Paths ending in ".." are changed to ".".  This is
             * tricky, but seems the easiest way to fix the problem.
             *
             * XXX
             * Since the first level MUST be FTS_ROOTLEVEL, base
             * is always initialized.
             */
            if (curr->fts_level == FTS_ROOTLEVEL) {
                if (type != DIR_TO_DNE) {
                    p = strrchr(curr->fts_path, '/');
                    base = (p == NULL) ? 0 :
                           (int)(p - curr->fts_path + 1);

                    if (!strcmp(&curr->fts_path[base],
                                ".."))
                        base += 1;
                } else
                    base = curr->fts_pathlen;
            }

            p = &curr->fts_path[base];
            nlen = curr->fts_pathlen - base;
            target_mid = to.target_end;
            if (*p != '/' && target_mid[-1] != '/')
                *target_mid++ = '/';
            *target_mid = 0;
            if (target_mid - to.p_path + nlen >= MAXPATHLEN) {
                warn("%s%s: name too long (not copied)",
                      to.p_path, p);
                badcp = rval = 1;
                continue;
            }
            (void)strncat(target_mid, p, nlen);
            to.p_end = target_mid + nlen;
            *to.p_end = 0;
            STRIP_TRAILING_SLASH(to);
        }

        if (curr->fts_info == FTS_DP) {
            /*
             * We are nearly finished with this directory.  If we
             * didn't actually copy it, or otherwise don't need to
             * change its attributes, then we are done.
             */
            if (!curr->fts_number)
                continue;
            /*
             * If -p is in effect, set all the attributes.
             * Otherwise, set the correct permissions, limited
             * by the umask.  Optimise by avoiding a chmod()
             * if possible (which is usually the case if we
             * made the directory).  Note that mkdir() does not
             * honour setuid, setgid and sticky bits, but we
             * normally want to preserve them on directories.
             */
            if (pflag) {
                if (setfile(curr->fts_statp, -1))
                    rval = 1;
                if (preserve_dir_acls(curr->fts_statp,
                                      curr->fts_accpath, to.p_path) != 0)
                    rval = 1;
            } else {
                mode = curr->fts_statp->st_mode;
                if ((mode & (S_ISUID | S_ISGID | S_ISVTX)) ||
                    ((mode | S_IRWXU) & mask) != (mode & mask))
                    if (chmod(to.p_path, mode & mask) != 0) {
                        warn("chmod: %s", to.p_path);
                        rval = 1;
                    }
            }
            continue;
        }

        /* Not an error but need to remember it happened. */
        if (stat(to.p_path, &to_stat) == -1)
            dne = 1;
        else {
            if (to_stat.st_ino == curr->fts_statp->st_ino) {
                warn("%s and %s are identical (not copied).",
                      to.p_path, curr->fts_path);
                badcp = rval = 1;
                if (S_ISDIR(curr->fts_statp->st_mode))
                    (void)fts_set(ftsp, curr, FTS_SKIP);
                continue;
            }
            if (!S_ISDIR(curr->fts_statp->st_mode) &&
                S_ISDIR(to_stat.st_mode)) {
                warn("cannot overwrite directory %s with "
                              "non-directory %s",
                      to.p_path, curr->fts_path);
                badcp = rval = 1;
                continue;
            }
            dne = 0;
        }

        switch (curr->fts_statp->st_mode & S_IFMT) {
            case S_IFLNK:
                /* Catch special case of a non-dangling symlink. */
                if ((fts_options & FTS_LOGICAL) ||
                    ((fts_options & FTS_COMFOLLOW) &&
                     curr->fts_level == 0)) {
                    if (copy_file(curr, dne))
                        badcp = rval = 1;
                } else {
                    if (copy_link(curr, !dne))
                        badcp = rval = 1;
                }
                break;
            case S_IFDIR:
                if (!Rflag) {
                    warn("%s is a directory (not copied).",
                          curr->fts_path);
                    (void)fts_set(ftsp, curr, FTS_SKIP);
                    badcp = rval = 1;
                    break;
                }
                /*
                 * If the directory doesn't exist, create the new
                 * one with the from file mode plus owner RWX bits,
                 * modified by the umask.  Trade-off between being
                 * able to write the directory (if from directory is
                 * 555) and not causing a permissions race.  If the
                 * umask blocks owner writes, we fail.
                 */
                if (dne) {
                    if (mkdir(to.p_path, curr->fts_statp->st_mode | S_IRWXU) < 0)
                        err("%s", to.p_path);
                } else if (!S_ISDIR(to_stat.st_mode)) {
                    errno = ENOTDIR;
                    err("%s", to.p_path);
                }
                /*
                 * Arrange to correct directory attributes later
                 * (in the post-order phase) if this is a new
                 * directory, or if the -p flag is in effect.
                 */
                curr->fts_number = pflag || dne;
                break;
            case S_IFBLK:
            case S_IFCHR:
                if (Rflag && !sflag) {
                    if (copy_special(curr->fts_statp, !dne))
                        badcp = rval = 1;
                } else {
                    if (copy_file(curr, dne))
                        badcp = rval = 1;
                }
                break;
            case S_IFSOCK:
                warn("%s is a socket (not copied).",
                      curr->fts_path);
                break;
            case S_IFIFO:
                if (Rflag && !sflag) {
                    if (copy_fifo(curr->fts_statp, !dne))
                        badcp = rval = 1;
                } else {
                    if (copy_file(curr, dne))
                        badcp = rval = 1;
                }
                break;
            default:
                if (copy_file(curr, dne))
                    badcp = rval = 1;
                break;
        }
        if (vflag && !badcp)
            printf("%s -> %s\n", curr->fts_path, to.p_path);
    }
    if (errno)
        err("fts_read");
    fts_close(ftsp);
    return (rval);
}