/*  Output errors to stderr and then exit. */ 

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "errors.h"
#include "utils.h"

/* get a string describing the errorcode */
const char *errortext (enum ERRORCODE err)
{
    switch (err) {
        case ERR_USAGE:
            return "wrong usage?";
            break;
        case ERR_IO_WRITE_FAIL:
        case ERR_IO_READ_FAIL:
            return "IO error occured";
            break;
        case ERR_UNKNOWN:
        default:
            return "unknown error";
            break;
    }
}

/* print an error message and exit with failure code */
void fatal (enum ERRORCODE err, const char *format, ...)
{
    if (err < 1 || err > 255) err = ERR_UNKNOWN;

    va_list args;
    va_start(args, format);

    /* print custom error message if supplied */
    if ( strnzero(format) )
        vfprintf(stderr, format, args);
    else
        fprintf(stderr, "[%d] %s\n", err, errortext(err));

    va_end(args);
    exit(err);
}