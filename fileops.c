#include "fileops.h"

/* openfile and readwrite need to know what to do */
enum IO_MODE { WRITE, READ };


/* open file and return file descriptor */
int openfile (enum IO_MODE mode, const char *fn)
{
    /* either read or write! */
    if (mode != WRITE && mode != READ)
        return -1; /* TODO */

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
        if (fd == -1) return -1; /* TODO */
        fcntl(fd, F_SETFD, 1);
        return fd;
    #endif
}


/* read up to buf_len bytes from file descriptor */
int readwrite (enum IO_MODE mode, int fd, void *buf, size_t buf_len)
{
    size_t chunk_len;
    char *bufptr = (char *)buf;

    while (buf_len > 0) {

        /* set chunk max. 1024 KiB */
        chunk_len = buf_len;
        if (chunk_len > 1048576)
            chunk_len = 1048576;
        
        if (mode == READ) {

            /* read next chunk */
            chunk_len = read(fd, bufptr, chunk_len);

            /* handle errors or blocks */
            if (chunk_len == 0) errno = EPROTO;
            if (chunk_len <= 0) {
                if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
                return -2; /* TODO */
            }

        } else if (mode == WRITE) {

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
                return -2; /* TODO */
            }

        } else return -3; /* TODO */

        /* increment pointer, decrement remaining length */
        bufptr += chunk_len;
        buf_len -= chunk_len;
    }
    return 0;
}

#define FILEOPS_CHUNKSIZE 1024

int read_to_buffer (int fd, struct buffer *filebuf)
{
    enum fileops_status e = FILEOPS_FAILURE;
    unsigned char readbuf[ FILEOPS_CHUNKSIZE ];
    size_t readlen;

    /* setup polling */
    struct pollfd polling;
    polling.fd = fd;
    polling.events = POLLIN | POLLERR; /* extend if writing too ! */
    

    for (;;) {
        /* read chunk to local buffer */
        readlen = read(fd, readbuf, sizeof readbuf);

        if (readlen == 0) { /* EOF */
            e = FILEOPS_SUCCESS;
            break;
        } else if (readlen < 0) { /* ERROR or BLOCKED */
            if (errno == EINTR) continue;
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
			    poll(&polling, 1, -1);
				continue;
			}
            e = FILEOPS_E_IOERROR;
            break;
        }

        /* put into buffer */
        if ((e = buffer_put(filebuf, readlen, readbuf)) != 0)
            break;
    }

    memzero(readbuf, sizeof readbuf);
    if (e != FILEOPS_SUCCESS) resetbuffer(filebuf);
    return e;
}


/* load a file to buffer */
int loadfile (const char *file, struct buffer **filebuf)
{
    enum fileops_status e = FILEOPS_FAILURE;
    int fd;

    if (filebuf == NULL || file == NULL)
        return FILEOPS_E_NULLPOINTER;
    *filebuf = NULL;
    
    /* open for reading */
    if ((fd = openfile(READ, file)) == -1)
        return FILEOPS_E_OPEN_READING;

    /* create a new buffer */ 
    if ((*filebuf = newbuffer()) == NULL)
        return FILEOPS_E_ALLOC_FAIL;

    /* read file */
    e = read_to_buffer(fd, *filebuf);
    close(fd);
    return e;
}

/* save a buffer to file */
int savefile (const char *file, void *buf, size_t buf_len)
{
    int fd, ret;
    
    /* open for writing */
    fd = openfile(WRITE, file);
    if (fd == -1) fatal(ERR_IO_READ_FAIL, NULL);

    /* write buffer to file and fsync */
    if ((ret = readwrite(WRITE, fd, buf, buf_len)) != -1)
        fsync(fd);
    close(fd);
    return ret;
}
