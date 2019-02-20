//
// Created by rebut_p on 17/02/19.
//

#include <stdio.h>
#include <filestream.h>
#include <unistd.h>

#include "extern.h"

int prn_normal(const char *s)
{
    return puts(s);
    /*int i, n;
    size_t clen;

    memset(&mbs, 0, sizeof(mbs));
    n = 0;
    while ((clen = mbrtowc(&wc, s, MB_LEN_MAX, &mbs)) != 0) {
        if (clen == (size_t) -2) {
            n += printf("%s", s);
            break;
        }
        if (clen == (size_t) -1) {
            memset(&mbs, 0, sizeof(mbs));
            putchar((unsigned char) *s);
            s++;
            n++;
            continue;
        }
        for (i = 0; i < (int) clen; i++)
            putchar((unsigned char) s[i]);
        s += clen;
        if (iswprint(wc))
            n += wcwidth(wc);
    }
    return (n);*/
}

int prn_printable(const char *s)
{
    return fputs(stdout, s);
    /*mbstate_t mbs;
    wchar_t wc;
    int i, n;
    size_t clen;

    memset(&mbs, 0, sizeof(mbs));
    n = 0;
    while ((clen = mbrtowc(&wc, s, MB_LEN_MAX, &mbs)) != 0) {
        if (clen == (size_t) -1) {
            putchar('?');
            s++;
            n++;
            memset(&mbs, 0, sizeof(mbs));
            continue;
        }
        if (clen == (size_t) -2) {
            putchar('?');
            n++;
            break;
        }
        if (!iswprint(wc)) {
            putchar('?');
            s += clen;
            n++;
            continue;
        }
        for (i = 0; i < (int) clen; i++)
            putchar((unsigned char) s[i]);
        s += clen;
        n += wcwidth(wc);
    }
    return (n);*/
}

/*
 * The fts system makes it difficult to replace fts_name with a different-
 * sized string, so we just calculate the real length here and do the
 * conversion in prn_octal()
 *
 * XXX when using f_octal_escape (-b) rather than f_octal (-B), the
 * length computed by len_octal may be too big. I just can't be buggered
 * to fix this as an efficient fix would involve a lookup table. Same goes
 * for the rather inelegant code in prn_octal.
 *
 *						DES 1998/04/23
 */

size_t len_octal(const char *s, int len)
{
    return write(1, s, len);
    /*mbstate_t mbs;
    wchar_t wc;
    size_t clen, r;

    memset(&mbs, 0, sizeof(mbs));
    r = 0;
    while (len != 0 && (clen = mbrtowc(&wc, s, len, &mbs)) != 0) {
        if (clen == (size_t) -1) {
            r += 4;
            s++;
            len--;
            memset(&mbs, 0, sizeof(mbs));
            continue;
        }
        if (clen == (size_t) -2) {
            r += 4 * len;
            break;
        }
        if (iswprint(wc))
            r++;
        else
            r += 4 * clen;
        s += clen;
    }
    return (r);*/
}

int prn_octal(const char *s)
{
    return fputs(stdout, s);
    /*static const char esc[] = "\\\\\"\"\aa\bb\ff\nn\rr\tt\vv";
    const char *p;
    mbstate_t mbs;
    wchar_t wc;
    size_t clen;
    unsigned char ch;
    int goodchar, i, len, prtlen;

    memset(&mbs, 0, sizeof(mbs));
    len = 0;
    while ((clen = mbrtowc(&wc, s, MB_LEN_MAX, &mbs)) != 0) {
        goodchar = clen != (size_t) -1 && clen != (size_t) -2;
        if (goodchar && iswprint(wc) && wc != L'\"' && wc != L'\\') {
            for (i = 0; i < (int) clen; i++)
                putchar((unsigned char) s[i]);
            len += wcwidth(wc);
        } else if (goodchar && f_octal_escape &&
                   #if WCHAR_MIN < 0
                   wc >= 0 &&
                   #endif
                   wc <= (wchar_t) UCHAR_MAX &&
                   (p = strchr(esc, (char) wc)) != NULL) {
            putchar('\\');
            putchar(p[1]);
            len += 2;
        } else {
            if (goodchar)
                prtlen = clen;
            else if (clen == (size_t) -1)
                prtlen = 1;
            else
                prtlen = strlen(s);
            for (i = 0; i < prtlen; i++) {
                ch = (unsigned char) s[i];
                putchar('\\');
                putchar('0' + (ch >> 6));
                putchar('0' + ((ch >> 3) & 7));
                putchar('0' + (ch & 7));
                len += 4;
            }
        }
        if (clen == (size_t) -2)
            break;
        if (clen == (size_t) -1) {
            memset(&mbs, 0, sizeof(mbs));
            s++;
        } else
            s += clen;
    }
    return (len);
     */
}

void usage(void)
{
    (void) fprintf(stderr,
#ifdef COLORLS
            "usage: ls [-ABCFGHILPRSTUWZabcdfghiklmnopqrstuwxy1,] [--color=when] [-D format]"
#else
                   "usage: ls [-ABCFHILPRSTUWZabcdfghiklmnopqrstuwxy1,] [-D format]"
                   #endif
                   " [file ...]\n");
    exit(1);
}

const char *user_from_uid(uid_t user, int t)
{
    static const char *root = "root";
    (void) t;
    (void) user;
    return root;
}

const char *group_from_gid(gid_t grp, int t)
{
    static const char *root = "root";
    (void) t;
    (void) grp;
    return root;
}
