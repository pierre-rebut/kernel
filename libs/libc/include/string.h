/*
* Copyright (c) LSE
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY LSE AS IS AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL LSE BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef STRING_H_
#define STRING_H_

#include <types.h>

#define MAX(a, b) ((a) > (b) ? (a): (b))
#define MIN(a, b) ((a) < (b) ? (a): (b))

int memcmp(const void *s1, const void *s2, u32 n);

void *memcpy(void *dest, const void *src, u32 n);

void *memmove(void *dest, const void *src, register u32 n);

void *memset(void *s, int c, u32 n);

int strcmp(const char *s1, const char *s2);

char *strcpy(char *dest, const char *src);

char *strdup(const char *s);

char *strcat(char *begin, char *end);

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

#endif                /* !STRING_H_ */
