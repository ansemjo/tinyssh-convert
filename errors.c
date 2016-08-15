#include <stdio.h>
#include <stdarg.h>

#include "errors.h"

void fatal(int errcode, const char *format, ...)
{
    if (errcode < 1 || errcode > 255) errcode = FAILURE;
    va_list args;
    va_start(args, format);
    printf("[%d] ", errcode);
    vprintf(format, args);
    va_end(args);
    exit(errcode);
}