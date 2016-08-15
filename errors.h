#ifndef _headerguard_errors_h_
#define _headerguard_errors_h_

#include <stdlib.h>

/* general errors */
#define FAILURE EXIT_FAILURE

/* file I/O */
#define IO_WRITE_FAIL 30

void fatal(int, const char *, ...);

#endif