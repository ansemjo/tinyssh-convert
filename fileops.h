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
};

/* chunk at once before putting it into buffer struct */
#define FILEOPS_CHUNKSIZE 1024

/* open file descriptors */
#define openwriting(file) open(file, O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC, 0644)
#define openreading(file) open(file, O_RDONLY | O_CLOEXEC)

/* functions passable to io function */
#define iowrite (ssize_t (*) (int, void *, size_t))write
#define ioread  read

/* lowlevel io */
int io (ssize_t (*rw) (int, void *, size_t), int fd, void *data, size_t datalen, size_t *iolenptr);

/* load and save files to/from buffer */
int loadfile (const char *file, struct buffer **filebuf);
int savefile (const char *file, struct buffer  *filebuf);

#endif