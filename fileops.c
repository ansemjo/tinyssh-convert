#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "fileops.h"

/* open file for reading */
int open_read (const char *fn)
{
    #ifdef O_CLOEXEC
        return open(fn, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    #else
        int fd = open(fn, O_RDONLY | O_NONBLOCK);
        if (fd == -1) return -1;
        fcntl(fd, F_SETFD, 1);
        return fd;
    #endif
}

/* open file for writing */
int open_write (const char *fn)
{
    #ifdef O_CLOEXEC
        return open(fn, O_CREAT | O_WRONLY | O_NONBLOCK | O_CLOEXEC, 0644);
    #else
        int fd = open(fn, O_CREAT | O_WRONLY | O_NONBLOCK, 0644);
        if (fd == -1) return -1;
        fcntl(fd, F_SETFD, 1);
        return fd;
    #endif
}

/* read up to buf_len bytes from fd */
int readall (int fd, void *buf, size_t buf_len)
{
    size_t chunk_len;
    unsigned char *bufptr = (unsigned char *)buf;

    while (buf_len > 0) {

        /* set chunk max. 1024 KiB */
        chunk_len = buf_len;
        if (chunk_len > 1048576)
            chunk_len = 1048576;
        
        /* read next chunk */
        chunk_len = read(fd, bufptr, chunk_len);

        /* handle errors or blocks */
        if (chunk_len == 0) errno = EPROTO;
        if (chunk_len <= 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
            return -1;
        }

        /* increment pointer, decrement remaining length */
        bufptr += chunk_len;
        buf_len -= chunk_len;
    }
    return 0;
}


/* load a file to buffer */
int loadfile (const char *file, void *buf, size_t buf_len)
{
    int fd, nbytes;

    /* open for reading */
    fd = open_read(file);
    if (fd == -1) return -1;

    /* read up to buf_len bytes,
    save number of bytes written to nbytes */
    nbytes = readall(fd, buf, buf_len);
    close(fd);
    return nbytes;
}
