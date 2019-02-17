//
// Created by rebut_p on 11/02/19.
//

#include <stdio.h>
#include <filestream.h>
#include <string.h>

int _input(FILE *stream, const char *format, va_list arglist);

int fscanf(FILE *stream, const char *fmt, ...) {
    int rc;
    va_list args;

    va_start(args, fmt);

    rc = _input(stream, fmt, args);

    va_end(args);

    return rc;
}

int vfscanf(FILE *stream, const char *fmt, va_list args) {
    return _input(stream, fmt, args);
}

int scanf(const char *fmt, ...) {
    int rc;
    va_list args;

    va_start(args, fmt);

    rc = _input(stdin, fmt, args);

    va_end(args);

    return rc;
}

int vscanf(const char *fmt, va_list args) {
    return _input(stdin, fmt, args);
}

int sscanf(const char *buffer, const char *fmt, ...) {
    int rc;
    va_list args;
    FILE *str = fopenBuf((char*) buffer, strlen(buffer));

    va_start(args, fmt);

    rc = _input(str, fmt, args);

    va_end(args);

    fclose(str);
    return rc;
}

int vsscanf(const char *buffer, const char *fmt, va_list args) {
    int rc;
    FILE *str = fopenBuf((char *)buffer, strlen(buffer));

    rc = _input(str, fmt, args);

    fclose(str);
    return rc;
}