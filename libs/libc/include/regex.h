//
// Created by rebut_p on 22/02/19.
//

#ifndef _REGEX_H
#define _REGEX_H

#include "ctype.h"

typedef long int s_reg_t;
typedef unsigned long int active_reg_t;

typedef unsigned long int reg_syntax_t;

extern reg_syntax_t re_syntax_options;

#define RE_DUP_MAX (0x7fff)

/* POSIX `cflags' bits (i.e., information for `regcomp').  */
/* If this bit is set, then use extended regular expression syntax.
   If not set, then use basic regular expression syntax.  */
#define REG_EXTENDED 1
/* If this bit is set, then ignore case when matching.
   If not set, then case is significant.  */
#define REG_ICASE (REG_EXTENDED << 1)
/* If this bit is set, then anchors do not match at newline
     characters in the string.
   If not set, then anchors do match at newlines.  */
#define REG_NEWLINE (REG_ICASE << 1)
/* If this bit is set, then report only success or fail in regexec.
   If not set, then returns differ between not matching and errors.  */
#define REG_NOSUB (REG_NEWLINE << 1)
/* POSIX `eflags' bits (i.e., information for regexec).  */
/* If this bit is set, then the beginning-of-line operator doesn't match
     the beginning of the string (presumably because it's not the
     beginning of a line).
   If not set, then the beginning-of-line operator does match the
     beginning of the string.  */
#define REG_NOTBOL 1
/* Like REG_NOTBOL, except for the end-of-line.  */
#define REG_NOTEOL (1 << 1)
/* Use PMATCH[0] to delimit the start and end of the search in the
   buffer.  */
#define REG_STARTEND (1 << 2)
/* If any error codes are removed, changed, or added, update the
   `re_error_msg' table in regex.c.  */


# define RE_BACKSLASH_ESCAPE_IN_LISTS ((unsigned long int) 1)
# define RE_BK_PLUS_QM (RE_BACKSLASH_ESCAPE_IN_LISTS << 1)
# define RE_CHAR_CLASSES (RE_BK_PLUS_QM << 1)
# define RE_CONTEXT_INDEP_ANCHORS (RE_CHAR_CLASSES << 1)
# define RE_CONTEXT_INDEP_OPS (RE_CONTEXT_INDEP_ANCHORS << 1)
# define RE_CONTEXT_INVALID_OPS (RE_CONTEXT_INDEP_OPS << 1)
# define RE_DOT_NEWLINE (RE_CONTEXT_INVALID_OPS << 1)
# define RE_DOT_NOT_NULL (RE_DOT_NEWLINE << 1)
# define RE_HAT_LISTS_NOT_NEWLINE (RE_DOT_NOT_NULL << 1)
# define RE_INTERVALS (RE_HAT_LISTS_NOT_NEWLINE << 1)
# define RE_LIMITED_OPS (RE_INTERVALS << 1)
# define RE_NEWLINE_ALT (RE_LIMITED_OPS << 1)
# define RE_NO_BK_BRACES (RE_NEWLINE_ALT << 1)
# define RE_NO_BK_PARENS (RE_NO_BK_BRACES << 1)
# define RE_NO_BK_REFS (RE_NO_BK_PARENS << 1)
# define RE_NO_BK_VBAR (RE_NO_BK_REFS << 1)
# define RE_NO_EMPTY_RANGES (RE_NO_BK_VBAR << 1)
# define RE_UNMATCHED_RIGHT_PAREN_ORD (RE_NO_EMPTY_RANGES << 1)
# define RE_NO_POSIX_BACKTRACKING (RE_UNMATCHED_RIGHT_PAREN_ORD << 1)
# define RE_NO_GNU_OPS (RE_NO_POSIX_BACKTRACKING << 1)
# define RE_DEBUG (RE_NO_GNU_OPS << 1)
# define RE_INVALID_INTERVAL_ORD (RE_DEBUG << 1)
# define RE_ICASE (RE_INVALID_INTERVAL_ORD << 1)
# define RE_CARET_ANCHORS_HERE (RE_ICASE << 1)
# define RE_CONTEXT_INVALID_DUP (RE_CARET_ANCHORS_HERE << 1)
# define RE_NO_SUB (RE_CONTEXT_INVALID_DUP << 1)


typedef enum
{
    REG_NOERROR = 0,        /* Success.  */
    REG_NOMATCH,                /* Didn't find a match (for regexec).  */
    /* POSIX regcomp return error codes.  (In the order listed in the
       standard.)  */
            REG_BADPAT,                /* Invalid pattern.  */
    REG_ECOLLATE,                /* Inalid collating element.  */
    REG_ECTYPE,                /* Invalid character class name.  */
    REG_EESCAPE,                /* Trailing backslash.  */
    REG_ESUBREG,                /* Invalid back reference.  */
    REG_EBRACK,                /* Unmatched left bracket.  */
    REG_EPAREN,                /* Parenthesis imbalance.  */
    REG_EBRACE,                /* Unmatched \{.  */
    REG_BADBR,                /* Invalid contents of \{\}.  */
    REG_ERANGE,                /* Invalid range end.  */
    REG_ESPACE,                /* Ran out of memory.  */
    REG_BADRPT,                /* No preceding re for repetition op.  */
    /* Error codes we've added.  */
            REG_EEND,                /* Premature end.  */
    REG_ESIZE,                /* Compiled pattern bigger than 2^16 bytes.  */
    REG_ERPAREN                /* Unmatched ) or \); not returned from regcomp.  */
} reg_errcode_t;

/* This data structure represents a compiled pattern.  Before calling
   the pattern compiler, the fields `buffer', `allocated', `fastmap',
   and `translate' can be set.  After the pattern has been compiled,
   the fields `re_nsub', `not_bol' and `not_eol' are available.  All
   other fields are private to the regex routines.  */

# define __RE_TRANSLATE_TYPE unsigned char *
# define RE_TRANSLATE_TYPE __RE_TRANSLATE_TYPE
# define REGS_REALLOCATE 1
# define REGS_UNALLOCATED 0
#  define RE_NREGS 30
# define REGS_FIXED 2

struct re_pattern_buffer
{
    /* Space that holds the compiled pattern.  It is declared as
       `unsigned char *' because its elements are sometimes used as
       array indexes.  */
    unsigned char *buffer;
    /* Number of bytes to which `buffer' points.  */
    unsigned long int allocated;
    /* Number of bytes actually used in `buffer'.  */
    unsigned long int used;
    /* Syntax setting with which the pattern was compiled.  */
    reg_syntax_t syntax;
    /* Pointer to a fastmap, if any, otherwise zero.  re_search uses the
       fastmap, if there is one, to skip over impossible starting points
       for matches.  */
    char *fastmap;
    /* Either a translate table to apply to all characters before
       comparing them, or zero for no translation.  The translation is
       applied to a pattern when it is compiled and to a string when it
       is matched.  */
    __RE_TRANSLATE_TYPE translate;
    /* Number of subexpressions found by the compiler.  */
    size_t re_nsub;
    /* Zero if this pattern cannot match the empty string, one else.
       Well, in truth it's used only in `re_search_2', to see whether or
       not we should use the fastmap, so we don't set this absolutely
       perfectly; see `re_compile_fastmap' (the `duplicate' case).  */
    unsigned can_be_null : 1;
    /* If REGS_UNALLOCATED, allocate space in the `regs' structure
       for `max (RE_NREGS, re_nsub + 1)' groups.
       If REGS_REALLOCATE, reallocate space if necessary.
       If REGS_FIXED, use what's there.  */
#ifdef __USE_GNU
# define REGS_UNALLOCATED 0
# define REGS_REALLOCATE 1
# define REGS_FIXED 2
#endif
    unsigned regs_allocated : 2;
    /* Set to zero when `regex_compile' compiles a pattern; set to one
       by `re_compile_fastmap' if it updates the fastmap.  */
    unsigned fastmap_accurate : 1;
    /* If set, `re_match_2' does not return information about
       subexpressions.  */
    unsigned no_sub : 1;
    /* If set, a beginning-of-line anchor doesn't match at the beginning
       of the string.  */
    unsigned not_bol : 1;
    /* Similarly for an end-of-line anchor.  */
    unsigned not_eol : 1;
    /* If true, an anchor at a newline matches.  */
    unsigned newline_anchor : 1;
};
typedef struct re_pattern_buffer regex_t;

/* Type for byte offsets within the string.  POSIX mandates this.  */
typedef int regoff_t;

/* POSIX specification for registers.  Aside from the different names than
   `re_registers', POSIX uses an array of structures, instead of a
   structure of arrays.  */
typedef struct
{
    regoff_t rm_so;  /* Byte offset from string's start to substring's start.  */
    regoff_t rm_eo;  /* Byte offset from string's start to substring's end.  */
} regmatch_t;

/* GCC 2.95 and later have "__restrict"; C99 compilers have
   "restrict", and "configure" may have defined "restrict".  */

/* gcc 3.1 and up support the [restrict] syntax.  */
#ifndef __restrict_arr
# if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)) \
 && !defined __GNUG__
#  define __restrict_arr __restrict
# else
#  define __restrict_arr
# endif
#endif

struct re_registers
{
  size_t num_regs;
  regoff_t *start;
  regoff_t *end;
};


/* POSIX compatibility.  */
extern int regcomp(regex_t *__restrict __preg,
                   const char *__restrict __pattern,
                   int __cflags);

extern int regexec(const regex_t *__restrict __preg,
                   const char *__restrict __string, size_t __nmatch,
                   regmatch_t __pmatch[__restrict_arr],
                   int __eflags);

extern size_t regerror(int __errcode, const regex_t *__restrict __preg,
                       char *__restrict __errbuf, size_t __errbuf_size);

extern void regfree(regex_t *__preg);

int re_search_2(struct re_pattern_buffer *bufp, const char *string1, int size1,
        const char *string2, int size2, int startpos, int range, struct re_registers *regs,int stop);

# define _RE_SYNTAX_POSIX_COMMON                                        \
  (RE_CHAR_CLASSES | RE_DOT_NEWLINE      | RE_DOT_NOT_NULL              \
   | RE_INTERVALS  | RE_NO_EMPTY_RANGES)

# define RE_SYNTAX_POSIX_BASIC                                          \
  (_RE_SYNTAX_POSIX_COMMON | RE_BK_PLUS_QM | RE_CONTEXT_INVALID_DUP)

# define RE_SYNTAX_POSIX_EXTENDED                                       \
  (_RE_SYNTAX_POSIX_COMMON  | RE_CONTEXT_INDEP_ANCHORS                  \
   | RE_CONTEXT_INDEP_OPS   | RE_NO_BK_BRACES                           \
   | RE_NO_BK_PARENS        | RE_NO_BK_VBAR                             \
   | RE_CONTEXT_INVALID_OPS | RE_UNMATCHED_RIGHT_PAREN_ORD)



#endif //_REGEX_H
