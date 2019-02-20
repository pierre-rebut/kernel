//
// Created by rebut_p on 11/02/19.
//

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <alloc.h>
#include <string.h>
#include "filestream.h"

#define STREAM_EOF 1
#define STREAM_ERROR 2

FILE *stdin;
FILE *stdout;
FILE *stderr;

FILE *fdopen(int fd)
{
    FILE *stream = malloc(sizeof(FILE));
    void *buf = malloc(FILE_BUFFER_SIZE);
    if (stream == NULL || buf == NULL) {
        free(stream);
        free(buf);
        return NULL;
    }

    stream->fd = fd;
    stream->buf = buf;
    stream->cnt = FILE_BUFFER_SIZE;
    stream->flags = 0;
    stream->posr = 0;
    stream->posw = 0;
    return stream;
}

FILE *fopen(const char *pathname, int flags, mode_t mode)
{
    int fd = open(pathname, flags, mode);
    if (fd == -1)
        return NULL;

    return fdopen(fd);
}

FILE *fopenBuf(char *buffer, u32 size)
{
    if (buffer == NULL || size == 0)
        return NULL;

    FILE *stream = malloc(sizeof(FILE));
    if (stream == NULL)
        return NULL;

    stream->fd = -1;
    stream->buf = buffer;
    stream->cnt = size;
    stream->flags = 0;
    stream->posr = 0;
    stream->posw = size;
    return stream;
}

int fclose(FILE *stream)
{
    if (stream == NULL)
        return -1;

    if (stream->fd >= 0)
        close(stream->fd);
    free(stream);
    return 0;
}

static int fileInternalRead(FILE *stream)
{
    char tmpBuf[FILE_BUFFER_SIZE];

    if (stream->fd == -1)
        return EOF;

    int size = read(stream->fd, tmpBuf, FILE_BUFFER_SIZE);
    if (size < 0) {
        stream->flags = STREAM_ERROR;
        return STREAM_ERROR;
    }

    if (size == 0) {
        stream->flags = STREAM_EOF;
        return EOF;
    }

    for (int i = 0; i < size; i++) {
        stream->buf[stream->posw] = tmpBuf[i];
        stream->posw = (stream->posw + 1) % stream->cnt;
    }

    return size;
}

int getc(FILE *stream)
{
    if (stream == NULL || stream->flags == STREAM_ERROR)
        return -1;

    if (stream->flags == STREAM_EOF)
        return EOF;

    if (stream->posr == stream->posw) {
        if (fileInternalRead(stream) == EOF)
            return EOF;
    }

    int tmp = stream->buf[stream->posr];
    stream->posr = (stream->posr + 1) % stream->cnt;
    return tmp;
}

int getchar()
{
    return getc(stdin);
}

int ungetc(int c, FILE *stream)
{
    if (stream == NULL || stream->flags == STREAM_ERROR)
        return -1;

    if (stream->posr == 0) {
        if (stream->posw == stream->cnt - 1)
            return -1;

        stream->posr = stream->cnt - 1;
    } else {
        if (stream->posw == stream->posr - 1)
            return -1;

        stream->posr -= 1;
    }

    stream->buf[stream->posr] = (char) c;

    return 0;
}

int ferror(FILE *stream)
{
    return (stream->flags == STREAM_ERROR);
}

int feof(FILE *stream)
{
    return (stream->flags == STREAM_EOF);
}

void clearerr(FILE *stream)
{
    stream->flags = 0;
}

size_t fread(char *buf, size_t size, size_t nmemb, FILE *stream)
{
    if (stream == NULL || stream->flags != 0)
        return 0;

    size_t readed = 0;
    for (size_t i = 0; i < nmemb; i++) {
        for (size_t pos = 0; pos < size; pos++) {
            if (stream->posr == stream->posw) {
                if (fileInternalRead(stream) <= 0)
                    return readed;
            }

            buf[i * size + pos] = stream->buf[stream->posr];
            stream->posr = (stream->posr + 1) % stream->cnt;
            readed += 1;
        }
    }

    return readed;
}

static int fileInternalWrite(FILE *stream)
{
    if (stream->fd == -1)
        return EOF;

    char tmpBuf[FILE_BUFFER_SIZE];
    u32 i = 0;

    while (i < FILE_BUFFER_SIZE) {
        if (stream->posr == stream->posw)
            break;

        tmpBuf[i] = stream->buf[stream->posr];
        stream->posr = (stream->posr + 1) % stream->cnt;
        i++;
    }

    // warn("file internal write: %d\n", i);

    int size = write(stream->fd, tmpBuf, i);
    if (size < 0) {
        stream->flags = STREAM_ERROR;
        return STREAM_ERROR;
    }

    if (size == 0) {
        stream->flags = STREAM_EOF;
        return EOF;
    }

    return size;
}

size_t fwrite(const char *buf, size_t size, size_t nmemb, FILE *stream)
{
    if (stream == NULL || stream->flags != 0)
        return 0;

    size_t writed = 0;
    for (size_t i = 0; i < nmemb; i++) {
        for (size_t pos = 0; pos < size; pos++) {
            if ((stream->posw + 1) % stream->cnt == stream->posr) {
                if (fileInternalWrite(stream) <= 0)
                    return writed;
            }

            stream->buf[stream->posw] = buf[i * size + pos];
            stream->posw = (stream->posw + 1) % stream->cnt;
            writed += 1;
        }
    }

    return writed;
}

int fprintf(FILE *stream, const char *fmt, ...)
{
    if (stream == NULL || stream->flags != 0)
        return -1;

    char printf_buf[1024];
    va_list args;
    int printed;

    va_start(args, fmt);
    printed = vsprintf(printf_buf, fmt, args);
    va_end(args);

    if (printed > 0)
        fwrite(printf_buf, (u32) printed, 1, stream);

    return printed;
}

int fputs(FILE *stream, const char *str) {
    if (stream == NULL || stream->flags != 0)
        return -1;

    return fwrite(str, strlen(str), 1, stream);
}

int fputchar(FILE *stream, char c)
{
    if (stream == NULL || stream->flags != 0)
        return -1;

    if ((stream->posw + 1) % stream->cnt == stream->posr) {
        if (fileInternalWrite(stream) <= 0)
            return 0;
    }

    stream->buf[stream->posw] = c;
    stream->posw = (stream->posw + 1) % stream->cnt;
    return 1;
}

int fflush(FILE *stream)
{
    if (stream == NULL || stream->flags != 0)
        return -1;

    return fileInternalWrite(stream);
}

static char *my_cat_len(FILE *stream, char *stock, int i)
{
    char *tmp;
    int compt;
    int x;
    int y;

    compt = 0;
    x = -1;
    y = -1;
    if (stock != NULL)
        while (stock[compt] != 0)
            compt++;
    if ((tmp = malloc(compt + i + 1)) == NULL)
        return (NULL);
    while (++x < compt)
        tmp[x] = stock[x];
    while (++y < i)
        tmp[x + y] = stream->buf[(stream->posr + y) % stream->cnt];
    tmp[x + y] = '\0';
    if (stock != NULL)
        free(stock);
    stream->posr = (stream->posr + i + 1) % stream->cnt;
    return (tmp);
}

char *getdelim(FILE *stream, char delim)
{
    if (stream == NULL || stream->flags != 0)
        return NULL;

    int i = 0;
    char *ret_line = NULL;

    while (42) {
        if (stream->posr == stream->posw) {
            if (fileInternalRead(stream) <= 0)
                return ret_line;
        }

        if (stream->buf[(stream->posr + i) % stream->cnt] == delim)
            return (my_cat_len(stream, ret_line, i));
        if ((stream->posr + i) % stream->cnt == stream->posw)
            ret_line = my_cat_len(stream, ret_line, i);
        i++;

    }
}

char *getline(FILE *stream)
{
    return getdelim(stream, '\n');
}