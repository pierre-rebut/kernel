
#include "proto.h"

#include <string.h>
#include <stdlib.h>

#ifdef ENABLE_UTF8
#include <wchar.h>
#include <wctype.h>

static bool use_utf8 = false;
		/* Whether we've enabled UTF-8 support. */

/* Enable UTF-8 support. */
void utf8_init(void)
{
	use_utf8 = true;
}

/* Is UTF-8 support enabled? */
bool using_utf8(void)
{
	return use_utf8;
}
#endif /* ENABLE_UTF8 */

/* Concatenate two allocated strings, and free the second. */
char *addstrings(char* str1, size_t len1, char* str2, size_t len2)
{
	str1 = charealloc(str1, len1 + len2 + 1);
	str1[len1] = '\0';

	strncat(&str1[len1], str2, len2);
	free(str2);

	return str1;
}

/* Return true if the value of c is in byte range, and false otherwise. */
bool is_byte(int c)
{
	return ((unsigned int)c == (unsigned char)c);
}

/* This function is equivalent to isalpha() for multibyte characters. */
bool is_alpha_mbchar(const char *c)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		wchar_t wc;

		if (mbtowc(&wc, c, MAXCHARLEN) < 0)
			return 0;

		return iswalpha(wc);
	} else
#endif
		return isalpha((unsigned char)*c);
}

/* This function is equivalent to isalnum() for multibyte characters. */
bool is_alnum_mbchar(const char *c)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		wchar_t wc;

		if (mbtowc(&wc, c, MAXCHARLEN) < 0)
			return 0;

		return iswalnum(wc);
	} else
#endif
		return isalnum((unsigned char)*c);
}

/* This function is equivalent to isblank() for multibyte characters. */
bool is_blank_mbchar(const char *c)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		wchar_t wc;

		if (mbtowc(&wc, c, MAXCHARLEN) < 0)
			return 0;

		return iswblank(wc);
	} else
#endif
		return isblank((unsigned char)*c);
}

/* This function is equivalent to iscntrl(), except in that it only
 * handles non-high-bit control characters. */
bool is_ascii_cntrl_char(int c)
{
	return (0 <= c && c < 32);
}

/* This function is equivalent to iscntrl() for multibyte characters,
 * except in that it also handles multibyte control characters with
 * their high bits set. */
bool is_cntrl_mbchar(const char *c)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		return ((c[0] & 0xE0) == 0 || c[0] == 127 ||
				((signed char)c[0] == -62 && (signed char)c[1] < -96));
	} else
#endif
		return (((unsigned char)*c & 0x60) == 0 || (unsigned char)*c == 127);
}

/* This function is equivalent to ispunct() for multibyte characters. */
bool is_punct_mbchar(const char *c)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		wchar_t wc;

		if (mbtowc(&wc, c, MAXCHARLEN) < 0)
			return 0;

		return iswpunct(wc);
	} else
#endif
		return ispunct((unsigned char)*c);
}

/* Return true when the given multibyte character c is a word-forming
 * character (that is: alphanumeric, or specified in wordchars, or
 * punctuation when allow_punct is true), and false otherwise. */
bool is_word_mbchar(const char *c, bool allow_punct)
{
	if (*c == '\0')
		return false;

	if (is_alnum_mbchar(c))
		return true;

	if (word_chars != NULL && *word_chars != '\0') {
		char symbol[MAXCHARLEN + 1];
		int symlen = parse_mbchar(c, symbol, NULL);

		symbol[symlen] = '\0';
		return (strstr(word_chars, symbol) != NULL);
	}

	return (allow_punct && is_punct_mbchar(c));
}

/* Return the visible representation of control character c. */
char control_rep(const signed char c)
{
	if (c == DEL_CODE)
		return '?';
	else if (c == -97)
		return '=';
	else if (c < 0)
		return c + 224;
	else
		return c + 64;
}

/* Return the visible representation of multibyte control character c. */
char control_mbrep(const char *c, bool isdata)
{
	/* An embedded newline is an encoded NUL if it is data. */
	if (*c == '\n' && (isdata || as_an_at))
		return '@';

#ifdef ENABLE_UTF8
	if (use_utf8) {
		if ((unsigned char)c[0] < 128)
			return control_rep(c[0]);
		else
			return control_rep(c[1]);
	} else
#endif
		return control_rep(*c);
}

/* Assess how many bytes the given (multibyte) character occupies.  Return -1
 * if the byte sequence is invalid, and return the number of bytes minus 8
 * when it encodes an invalid codepoint.  Also, in the second parameter,
 * return the number of columns that the character occupies. */
int length_of_char(const char *c, int *width)
{
    (void) c;
    (void) width;
#ifdef ENABLE_UTF8
	if (use_utf8) {
		wchar_t wc;
		int charlen = mbtowc(&wc, c, MAXCHARLEN);

		/* If the sequence is invalid... */
		if (charlen < 0)
			return -1;

		/* If the codepoint is invalid... */
		if (!is_valid_unicode(wc))
			return charlen - 8;
		else {
			*width = wcwidth(wc);
			/* If the codepoint is unassigned, assume a width of one. */
			if (*width < 0)
				*width = 1;
			return charlen;
		}
	} else
#endif
		return 1;
}

/* This function is equivalent to wcwidth() for multibyte characters. */
int mbwidth(const char *c)
{
    (void) c;
#ifdef ENABLE_UTF8
	if (use_utf8) {
		wchar_t wc;
		int width;

		if (mbtowc(&wc, c, MAXCHARLEN) < 0)
			return 1;

		width = wcwidth(wc);

		if (width == -1)
			return 1;

		return width;
	} else
#endif
		return 1;
}

/* Convert the Unicode value in chr to a multibyte character, if possible.
 * If the conversion succeeds, return the (dynamically allocated) multibyte
 * character and its length.  Otherwise, return an undefined (dynamically
 * allocated) multibyte character and a length of zero. */
char *make_mbchar(long chr, int *chr_mb_len)
{
	char *chr_mb;

#ifdef ENABLE_UTF8
	if (use_utf8) {
		chr_mb = charalloc(MAXCHARLEN);
		*chr_mb_len = wctomb(chr_mb, (wchar_t)chr);

		/* Reject invalid Unicode characters. */
		if (*chr_mb_len < 0 || !is_valid_unicode((wchar_t)chr)) {
			IGNORE_CALL_RESULT(wctomb(NULL, 0));
			*chr_mb_len = 0;
		}
	} else
#endif
	{
		*chr_mb_len = 1;
		chr_mb = mallocstrncpy(NULL, (char *)&chr, 1);
	}

	return chr_mb;
}

/* Parse a multibyte character from buf.  Return the number of bytes
 * used.  If chr isn't NULL, store the multibyte character in it.  If
 * col isn't NULL, add the character's width (in columns) to it. */
int parse_mbchar(const char *buf, char *chr, size_t *col)
{
	int length;

#ifdef ENABLE_UTF8
	/* If this is a UTF-8 starter byte, get the number of bytes of the character. */
	if ((signed char)*buf < 0) {
		length = mblen(buf, MAXCHARLEN);

		/* When the multibyte sequence is invalid, only take the first byte. */
		if (length <= 0)
			length = 1;
	} else
#endif
		length = 1;

	/* When requested, store the multibyte character in chr. */
	if (chr != NULL)
		for (int i = 0; i < length; i++)
			chr[i] = buf[i];

	/* When requested, add the width of the character to col. */
	if (col != NULL) {
		/* If we have a tab, compute its width in columns based on the
		 * current value of col. */
		if (*buf == '\t')
			*col += tabsize - *col % tabsize;
		/* If we have a control character, it's two columns wide: one
		 * column for the "^", and one for the visible character. */
		else if (is_cntrl_mbchar(buf))
			*col += 2;
		/* If we have a normal character, get its width normally. */
		else if (length == 1)
			*col += 1;
#ifdef ENABLE_UTF8
		else
			*col += mbwidth(buf);
#endif
	}

	return length;
}

/* Return the index in buf of the beginning of the multibyte character
 * before the one at pos. */
size_t move_mbleft(const char *buf, size_t pos)
{
    (void) buf;
#ifdef ENABLE_UTF8
	if (use_utf8) {
		size_t before, char_len = 0;

		if (pos < 4)
			before = 0;
		else {
			const char *ptr = buf + pos;

			/* Probe for a valid starter byte in the preceding four bytes. */
			if ((signed char)*(--ptr) > -65)
				before = pos - 1;
			else if ((signed char)*(--ptr) > -65)
				before = pos - 2;
			else if ((signed char)*(--ptr) > -65)
				before = pos - 3;
			else if ((signed char)*(--ptr) > -65)
				before = pos - 4;
			else
				before = pos - 1;
		}

		/* Move forward again until we reach the original character,
		 * so we know the length of its preceding character. */
		while (before < pos) {
			char_len = parse_mbchar(buf + before, NULL, NULL);
			before += char_len;
		}

		return before - char_len;
	} else
#endif
	return (pos == 0 ? 0 : pos - 1);
}

/* Return the index in buf of the beginning of the multibyte character
 * after the one at pos. */
size_t move_mbright(const char *buf, size_t pos)
{
	return pos + parse_mbchar(buf + pos, NULL, NULL);
}

/* This function is equivalent to strcasecmp() for multibyte strings. */
int mbstrcasecmp(const char *s1, const char *s2)
{
	return mbstrncasecmp(s1, s2, HIGHEST_POSITIVE);
}

/* This function is equivalent to strncasecmp() for multibyte strings. */
int mbstrncasecmp(const char *s1, const char *s2, size_t n)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		wchar_t wc1, wc2;

		while (*s1 != '\0' && *s2 != '\0' && n > 0) {
			bool bad1 = (mbtowc(&wc1, s1, MAXCHARLEN) < 0);
			bool bad2 = (mbtowc(&wc2, s2, MAXCHARLEN) < 0);

			if (bad1 || bad2) {
				if (*s1 != *s2)
					return (unsigned char)*s1 - (unsigned char)*s2;

				if (bad1 != bad2)
					return (bad1 ? 1 : -1);
			} else {
				int difference = towlower(wc1) - towlower(wc2);

				if (difference != 0)
					return difference;
			}

			s1 += move_mbright(s1, 0);
			s2 += move_mbright(s2, 0);
			n--;
		}

		return (n > 0) ? ((unsigned char)*s1 - (unsigned char)*s2) : 0;
	} else
#endif
		return strncasecmp(s1, s2, n);
}

/* This function is equivalent to strcasestr() for multibyte strings. */
char *mbstrcasestr(const char *haystack, const char *needle)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		size_t needle_len = mbstrlen(needle);

		while (*haystack != '\0') {
			if (mbstrncasecmp(haystack, needle, needle_len) == 0)
				return (char *)haystack;

			haystack += move_mbright(haystack, 0);
		}

		return NULL;
	} else
#endif
		return (char *) strcasestr(haystack, needle);
}

/* This function is equivalent to strstr(), except in that it scans the
 * string in reverse, starting at pointer. */
char *revstrstr(const char *haystack, const char *needle,
		const char *pointer)
{
	size_t needle_len = strlen(needle);
	size_t tail_len = strlen(pointer);

	if (tail_len < needle_len)
		pointer += tail_len - needle_len;

	while (pointer >= haystack) {
		if (strncmp(pointer, needle, needle_len) == 0)
			return (char *)pointer;
		pointer--;
	}

	return NULL;
}

/* This function is equivalent to strcasestr(), except in that it scans
 * the string in reverse, starting at pointer. */
char *revstrcasestr(const char *haystack, const char *needle,
		const char *pointer)
{
	size_t needle_len = strlen(needle);
	size_t tail_len = strlen(pointer);

	if (tail_len < needle_len)
		pointer += tail_len - needle_len;

	while (pointer >= haystack) {
		if (strncasecmp(pointer, needle, needle_len) == 0)
			return (char *)pointer;
		pointer--;
	}

	return NULL;
}

/* This function is equivalent to strcasestr() for multibyte strings,
 * except in that it scans the string in reverse, starting at pointer. */
char *mbrevstrcasestr(const char *haystack, const char *needle,
		const char *pointer)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		size_t needle_len = mbstrlen(needle);
		size_t tail_len = mbstrlen(pointer);

		if (tail_len < needle_len)
			pointer += tail_len - needle_len;

		if (pointer < haystack)
			return NULL;

		while (true) {
			if (mbstrncasecmp(pointer, needle, needle_len) == 0)
				return (char *)pointer;

			if (pointer == haystack)
				return NULL;

			pointer = haystack + move_mbleft(haystack, pointer - haystack);
		}
	} else
#endif
		return revstrcasestr(haystack, needle, pointer);
}

/* Count the number of (multibyte) characters in the given string. */
size_t mbstrlen(const char *s)
{
	size_t maxlen = (size_t)-1;

#ifdef ENABLE_UTF8
	if (use_utf8) {
		size_t n = 0;

		while (*s != '\0' && maxlen > 0) {
			if ((signed char)*s < 0) {
				int length = mblen(s, MAXCHARLEN);

				s += (length < 0 ? 1 : length);
			} else
				s++;

			maxlen--;
			n++;
		}

		return n;
	} else
#endif
		return strnlen(s, maxlen);
}

#if !defined(NANO_TINY) || defined(ENABLE_JUSTIFY)
/* This function is equivalent to strchr() for multibyte strings. */
char *mbstrchr(const char *s, const char *c)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		bool bad_s_mb = false, bad_c_mb = false;
		char symbol[MAXCHARLEN];
		const char *q = s;
		wchar_t ws, wc;

		if (mbtowc(&wc, c, MAXCHARLEN) < 0) {
			wc = (unsigned char)*c;
			bad_c_mb = true;
		}

		while (*s != '\0') {
			int sym_len = parse_mbchar(s, symbol, NULL);

			if (mbtowc(&ws, symbol, sym_len) < 0) {
				ws = (unsigned char)*s;
				bad_s_mb = true;
			}

			if (bad_s_mb == bad_c_mb && ws == wc)
				break;

			s += sym_len;
			q += sym_len;
		}

		if (*s == '\0')
			q = NULL;

		return (char *)q;
	} else
#endif
		return (char *) strchr(s, *c);
}
#endif /* !NANO_TINY || ENABLE_JUSTIFY */

#ifndef NANO_TINY
/* This function is equivalent to strpbrk() for multibyte strings. */
char *mbstrpbrk(const char *s, const char *accept)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		for (; *s != '\0'; s += move_mbright(s, 0)) {
			if (mbstrchr(accept, s) != NULL)
				return (char *)s;
		}

		return NULL;
	} else
#endif
		return strpbrk((char*)s, (char*) accept);
}

/* Locate, in the string that starts at head, the first occurrence of any of
 * the characters in the string accept, starting from pointer and searching
 * backwards. */
char *revstrpbrk(const char *head, const char *accept, const char *pointer)
{
	if (*pointer == '\0') {
		if (pointer == head)
			return NULL;
		pointer--;
	}

	while (pointer >= head) {
		if (strchr(accept, *pointer) != NULL)
			return (char *)pointer;
		pointer--;
	}

	return NULL;
}

/* The same as the preceding function but then for multibyte strings. */
char *mbrevstrpbrk(const char *head, const char *accept, const char *pointer)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		if (*pointer == '\0') {
			if (pointer == head)
				return NULL;
			pointer = head + move_mbleft(head, pointer - head);
		}

		while (true) {
			if (mbstrchr(accept, pointer) != NULL)
				return (char *)pointer;

			/* If we've reached the head of the string, we found nothing. */
			if (pointer == head)
				return NULL;

			pointer = head + move_mbleft(head, pointer - head);
		}
	} else
#endif
		return revstrpbrk(head, accept, pointer);
}
#endif /* !NANO_TINY */

#if defined(ENABLE_NANORC) && (!defined(NANO_TINY) || defined(ENABLE_JUSTIFY))
/* Return true if the string s contains one or more blank characters,
 * and false otherwise. */
bool has_blank_chars(const char *s)
{
	for (; *s != '\0'; s++) {
		if (isblank((unsigned char)*s))
			return true;
	}

	return false;
}

/* Return true if the multibyte string s contains one or more blank
 * multibyte characters, and false otherwise. */
bool has_blank_mbchars(const char *s)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		char symbol[MAXCHARLEN];

		for (; *s != '\0'; s += move_mbright(s, 0)) {
			parse_mbchar(s, symbol, NULL);

			if (is_blank_mbchar(symbol))
				return true;
		}

		return false;
	} else
#endif
		return has_blank_chars(s);
}
#endif /* ENABLE_NANORC && (!NANO_TINY || ENABLE_JUSTIFY) */

#ifdef ENABLE_UTF8
/* Return true if wc is valid Unicode, and false otherwise. */
bool is_valid_unicode(wchar_t wc)
{
	return ((0 <= wc && wc <= 0xD7FF) ||
				(0xE000 <= wc && wc <= 0xFDCF) ||
				(0xFDF0 <= wc && wc <= 0xFFFD) ||
				(0xFFFF < wc && wc <= 0x10FFFF && (wc & 0xFFFF) <= 0xFFFD));
}
#endif
