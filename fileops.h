#ifndef _headerguard_fileops_h_
#define _headerguard_fileops_h_

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
};

/* chunk at once before putting it into buffer struct */
#define FILEOPS_CHUNKSIZE 1024

/* open file descriptors */
extern int openwriting (const char *file);
extern int openreading (const char *file);

/* functions passable to io function (casting the const on write)*/
#define iowrite   (ssize_t (*) (int, void *, size_t))  write
#define ioread  /*(ssize_t (*) (int, void *, size_t))*/read

/* lowlevel io */
extern int io (ssize_t (*rw) (int, void *, size_t), int fd, void *data, size_t datalen, size_t *iolenptr);

/* load and save files to/from buffer */
extern int loadfile   (const char *file, struct buffer **filebuf);
extern int savefile   (const char *file, struct buffer  *filebuf);
extern int savestring (const char *file, unsigned char *string, size_t stringlen);

#endif /* _headerguard_fileops_h_ */