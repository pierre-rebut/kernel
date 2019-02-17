//
// Created by rebut_p on 11/02/19.
//

#ifndef KERNEL_FILESTREAM_H
#define KERNEL_FILESTREAM_H

#define FILE_BUFFER_SIZE 4096

#include <types.h>

typedef struct {
    int fd;
    int flags;

    int cnt;
    char *buf;

    int posr;
    int posw;
} FILE;

FILE *fdopen(int fd);
FILE *fopen(const char *pathname, int flags, mode_t mode);
FILE *fopenBuf(char *buffer, u32 size);
int fclose(FILE *stream);
int getc(FILE *stream);
int getchar();
int ungetc(int c, FILE *stream);
int ferror(FILE *stream);
int feof(FILE *stream);
void clearerr(FILE *stream);
int fflush(FILE *stream);

size_t fread(char *buf, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const char *buf, size_t size, size_t nmemb, FILE *stream);
int fprintf(FILE *stream, const char *fmt, ...);
int fputchar(FILE *stream, char c);

char *getline(FILE *stream);
char *getdelim(FILE *stream, char delim);

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

#endif //KERNEL_FILESTREAM_H
