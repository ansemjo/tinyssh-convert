#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <fcntl.h>

#include "fileops.h"
#include "errors.h"

/* open file and return file descriptor */
int openfile (enum OPENMODE mode, const char *fn)
{
    /* either read or write! */
    if (mode != ( WRITE | READ ) ) return -1;

    #ifdef O_CLOEXEC
        switch (mode) {
            case WRITE: return open(fn, O_CREAT | O_WRONLY | O_NONBLOCK | O_CLOEXEC, 0644);
            case READ:  return open(fn, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        }        
    #else
        int fd;
        switch (mode) {
            case WRITE: fd = open(fn, O_CREAT | O_WRONLY | O_NONBLOCK, 0644);
            case READ:  fd = open(fn, O_RDONLY | O_NONBLOCK);
        }
        if (fd == -1) return -1;
        fcntl(fd, F_SETFD, 1);
        return fd;
    #endif
}

/* read up to buf_len bytes from file descriptor */
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
            return -2;
        }

        /* increment pointer, decrement remaining length */
        bufptr += chunk_len;
        buf_len -= chunk_len;
    }
    return 0;
}

/* write all to file descriptor */
int writeall (int fd, const void *buf, size_t buf_len)
{
    size_t chunk_len;
    char *bufptr = (char *)buf;

    while (buf_len > 0) {

        /* set chunk max. 1024 KiB */
        chunk_len = buf_len;
        if (chunk_len > 1048576)
            chunk_len = 1048576;

        /* write next chunk */
        chunk_len = write(fd, buf, chunk_len);

        /* handle errors or blocks */
        if (chunk_len < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                struct pollfd p;
                p.fd = fd;
                p.events = POLLOUT | POLLERR;
                poll(&p,1,-1);
                continue;
            }
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
    int fd, ret;

    /* open for reading */
    fd = openfile(READ, file);
    if (fd == -1) fatal(ERR_IO_READ_FAIL, NULL);

    /* read up to buf_len bytes */
    ret = readall(fd, buf, buf_len);
    close(fd);
    return ret;
}

/* save a buffer to file */
int savefile (const char *file, const void *buf, size_t buf_len)
{
    int fd, ret;
    
    /* open for writing */
    fd = openfile(WRITE, file);
    if (fd == -1) fatal(ERR_IO_READ_FAIL, NULL);

    /* write buffer to file and fsync */
    if ((ret = writeall(fd, buf, buf_len)) != -1)
        fsync(fd);
    close(fd);
    return ret;
}
