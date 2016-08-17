#ifndef _headerguard_errors_h_
#define _headerguard_errors_h_

#include "defines.h"

enum ERRORCODE {
    ERR_UNKNOWN = 1,
    ERR_USAGE,

    ERR_IO_WRITE_FAIL,
    ERR_IO_READ_FAIL,

    filename_too_long,
};

void fatal (enum ERRORCODE, const char *, ...);
const char *errortext (enum ERRORCODE);
void usage ();

#endif