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
    FILEOPS_SUCCESS =    0,
    FILEOPS_FAILURE = -100,
 /* general errors */
    FILEOPS_NULLPOINTER,
 /* handling files */
    FILEOPS_CANNOT_OPEN_READING,
    FILEOPS_CANNOT_OPEN_WRITING,
    FILEOPS_IOERROR,
    FILEOPS_INCOMPLETE_WRITE,
 /* buffer related */
    FILEOPS_ALLOCATION_FAIL,
    FILEOPS_READ_ZERO_BYTES,
};

/* constants */
#define FILEOPS_CHUNKSIZE 1024  /* how much to read at once before putting it into buffer struct */

/* load and save files to/from buffer */
int loadfile (const char *file, struct buffer **filebuf);
int savefile (const char *file, struct buffer  *filebuf);

#endif