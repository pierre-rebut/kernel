//
// Created by rebut_p on 17/02/19.
//

#include <fts.h>
#include <string.h>

#include "ls.h"
#include "extern.h"

int namecmp(const FTSENT *a, const FTSENT *b)
{

    return (strcmp(a->fts_name, b->fts_name));
}

int revnamecmp(const FTSENT *a, const FTSENT *b)
{

    return (strcmp(b->fts_name, a->fts_name));
}

int modcmp(const FTSENT *a, const FTSENT *b)
{

    if (b->fts_statp->st_mtim > a->fts_statp->st_mtim)
        return (1);
    if (b->fts_statp->st_mtim < a->fts_statp->st_mtim)
        return (-1);

    if (f_samesort)
        return (strcmp(b->fts_name, a->fts_name));
    else
        return (strcmp(a->fts_name, b->fts_name));
}

int revmodcmp(const FTSENT *a, const FTSENT *b)
{
    return (modcmp(b, a));
}

int acccmp(const FTSENT *a, const FTSENT *b)
{

    if (b->fts_statp->st_atim > a->fts_statp->st_atim)
        return (1);
    if (b->fts_statp->st_atim < a->fts_statp->st_atim)
        return (-1);

    if (f_samesort)
        return (strcmp(b->fts_name, a->fts_name));
    else
        return (strcmp(a->fts_name, b->fts_name));
}

int revacccmp(const FTSENT *a, const FTSENT *b)
{
    return (acccmp(b, a));
}

int birthcmp(const FTSENT *a, const FTSENT *b)
{
    return statcmp(a, b);
}

int revbirthcmp(const FTSENT *a, const FTSENT *b)
{
    return (birthcmp(b, a));
}

int statcmp(const FTSENT *a, const FTSENT *b)
{

    if (b->fts_statp->st_ctim > a->fts_statp->st_ctim)
        return (1);
    if (b->fts_statp->st_ctim < a->fts_statp->st_ctim)
        return (-1);

    if (f_samesort)
        return (strcmp(b->fts_name, a->fts_name));
    else
        return (strcmp(a->fts_name, b->fts_name));
}

int revstatcmp(const FTSENT *a, const FTSENT *b)
{
    return (statcmp(b, a));
}

int sizecmp(const FTSENT *a, const FTSENT *b)
{

    if (b->fts_statp->st_size > a->fts_statp->st_size)
        return (1);
    if (b->fts_statp->st_size < a->fts_statp->st_size)
        return (-1);
    return (strcmp(a->fts_name, b->fts_name));
}

int revsizecmp(const FTSENT *a, const FTSENT *b)
{
    return (sizecmp(b, a));
}