//
// Created by rebut_p on 17/02/19.
//

#include <string.h>
#include <kstd.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <err.h>
#include <time.h>
#include <sys/stat.h>

char *getenv(const char *name)
{
    if (name == NULL)
        return NULL;

    char **env = (char **) 0x1501000;
    u32 size = strlen(name);

    while (*env != NULL) {
        if (strncmp(name, *env, size) == 0)
            return (*env) + size + 1;

        env++;
    }

    return NULL;
}

mode_t umask(mode_t mode) {
    static mode_t curMask = O_RDONLY;
    mode_t tmp = curMask;
    curMask = mode;
    return tmp;
}

int access(const char *name, int amode) {
    struct stat tmp;
    if (stat(name, &tmp) == -1)
        return -1;

    return ((tmp.st_mode & amode) == amode);
}

static const char letters[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

static int __gen_tempname(char *tmpl, int suffixlen, int flags, int kind)
{
    int len;
    char *XXXXXX;
    static u32 value;
    u32 random_time_bits;
    unsigned int count;
    int fd = -1;
    int save_errno = errno;
    /* A lower bound on the number of temporary files to attempt to
       generate.  The maximum total number of temporary file names that
       can exist for a given template is 62**6.  It should never be
       necessary to try all of these combinations.  Instead if a reasonable
       number of names is tried (we define reasonable as 62**3) fail to
       give the system administrator the chance to remove the problems.  */
#define ATTEMPTS_MIN (62 * 62 * 62)
    /* The number of times to attempt to generate a temporary file.  To
       conform to POSIX, this must be no smaller than TMP_MAX.  */
#if ATTEMPTS_MIN < TMP_MAX
    unsigned int attempts = TMP_MAX;
#else
    unsigned int attempts = ATTEMPTS_MIN;
#endif
    len = strlen (tmpl);
    if (len < 6 + suffixlen || memcmp (&tmpl[len - 6 - suffixlen], "XXXXXX", 6))
    {
        errno = EINVAL;
        return -1;
    }
    /* This is where the Xs start.  */
    XXXXXX = &tmpl[len - 6 - suffixlen];
    /* Get some more or less random data.  */
#ifdef RANDOM_BITS
    RANDOM_BITS (random_time_bits);
#else
    {
        time_t tv = time(NULL);
        random_time_bits = ((u32) tv << 16) ^ tv;
    }
#endif
    value += random_time_bits ^ getpid ();
    for (count = 0; count < attempts; value += 7777, ++count)
    {
        u32 v = value;
        /* Fill in the random bits.  */
        XXXXXX[0] = letters[v % 62];
        v /= 62;
        XXXXXX[1] = letters[v % 62];
        v /= 62;
        XXXXXX[2] = letters[v % 62];
        v /= 62;
        XXXXXX[3] = letters[v % 62];
        v /= 62;
        XXXXXX[4] = letters[v % 62];
        v /= 62;
        XXXXXX[5] = letters[v % 62];
        switch (kind)
        {
            case 0:
                fd = open(tmpl, (flags & ~O_ACCMODE) | O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
                break;
            case 1:
                fd = mkdir (tmpl, S_IRUSR | S_IWUSR | S_IXUSR);
                break;
            default:
               err("error gen template\n");
        }
        if (fd >= 0)
        {
            errno = save_errno;
            return fd;
        }
        else if (errno != EEXIST)
            return -1;
    }
    /* We got out of the loop because we ran out of combinations to try.  */
    errno = EEXIST;
    return -1;
}

int mkstemp(char *template)
{
    return __gen_tempname (template, 0, 0, 0);
}