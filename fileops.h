#ifndef _headerguard_fileops_h_
#define _headerguard_fileops_h_

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <fcntl.h>

#include "errors.h"
#include "buffer.h"

/* status codes */
enum fileops_status {
    FILEOPS_SUCCESS = 0,
    FILEOPS_FAILURE = 1,
 /* general errors */
    FILEOPS_E_NULLPOINTER,
 /* handling files */
    FILEOPS_E_OPEN_READING,
    FILEOPS_E_OPEN_WRITING,
    FILEOPS_E_IOERROR,
 /* buffer related */
    FILEOPS_E_ALLOC_FAIL,
    FILEOPS_E_READ_ZERO_BYTES,
};

/* load and save files to/from buffer */
int loadfile (const char *file, struct buffer **filebuf);
int savefile (const char *file, void *buf, size_t buf_len);

#endif