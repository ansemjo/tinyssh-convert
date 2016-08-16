/*  Output errors to stderr and then exit. */ 

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "errors.h"

/* get a string describing the errorcode */
const char *errcode(enum errorcode err)
{
    switch (err) {
        case ERR_USAGE:
            return "wrong usage?";
            break;
        case ERR_IO_WRITE_FAIL:
        case ERR_IO_READ_FAIL:
            return "IO error occured";
            break;
        default:
            return "unknown error";
            break;
    }
}

/* print an error message and exit with failure code */
void fatal(enum errorcode err, const char *format, ...)
{
    if (err < 1 || err > 255) err = ERR_UNKNOWN;

    va_list args;
    va_start(args, format);

    /* see if custom error message supplied */
    if (format == NULL || strlen(format) == 0)
        fprintf(stderr, "[%d] %s\n", err, errcode(err));
    else
        vfprintf(stderr, format, args);
    va_end(args);
    exit(err);
}