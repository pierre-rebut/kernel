//
// Created by rebut_p on 23/02/19.
//

#ifndef _KERNEL_STRING_H
#define _KERNEL_STRING_H

#include "ctype.h"

#define MAX(a, b) ((a) > (b) ? (a): (b))
#define MIN(a, b) ((a) < (b) ? (a): (b))

int memcmp(const void *s1, const void *s2, u32 n);

void *memcpy(void *dest, const void *src, u32 n);

void *memmove(void *dest, const void *src, u32 n);

void *memset(void *s, int c, u32 n);

int strcmp(const char *s1, const char *s2);

char *strcpy(char *dest, const char *src);

u32 strlen(const char *s);

u32 strnlen(const char *s, u32 maxlen);

char *strncpy(char *dest, const char *src, u32 n);

int strncmp(const char *s1, const char *s2, u32 n);

char *strdup(const char *str);

char *strchr(const char *p, int ch);

char *strtok(char *s, const char *delim);

u32 strcspn(const char *s1, register const char *s2);

int strcontain(const char *s1, char c);

int strncontain(const char *s, char c, u32 n);

void strtoupper(char *str);

u32 str_backspace(char *str, char c, char **);

u32 strsplit(char *str, char delim);

u32 str_begins_with(const char *str, const char *with);

const char *strstr(register const char *string, char *substring);

char *strrchr(const char *cp, int ch);

size_t strlcpy(char *dst, const char *src, size_t siz);

char *strncat(char *s1, const char *s2, size_t n);

void strmode(register mode_t mode, register char *p);

char *strpbrk(register char *string, char *chars);

int strcasecmp(register const char *s1, register const char *s2);

int strncasecmp(register const char *s1, register const char *s2, size_t length);

const char *strcasestr(register const char *string, const char *substring);

char *dirname(char *str);

char *basename (const char *filename);

void *memchr(const void *s, int c, size_t n);

#endif //_KERNEL_STRING_H
