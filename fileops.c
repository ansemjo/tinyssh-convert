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

int io (ssize_t (*rw) (int, void *, size_t), int fd, void *data, size_t datalen, size_t *iolenptr)
{
    size_t iolen = 0;
    ssize_t iochunk;

    if (iolenptr != NULL)
        *iolenptr = 0;
 
    /* set up polling */
    struct pollfd polling;
    polling.fd = fd;
    polling.events = POLLERR | rw == read ? POLLIN : POLLOUT;
    
    while (datalen > iolen) {
 
        /* read or write next chunk */
        iochunk = (rw) (fd, ((char *)data) + iolen, datalen - iolen);
 
        /* ERROR or BLOCKED */
        if (iochunk < 0) {
            /* interrupted */
            if (errno == EINTR)
                continue;
            /* would be blocked */
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
			    poll(&polling, 1, -1);
				continue;
			}
            /* other error */
            return FILEOPS_IOERROR;
        
        /* some data was processed */
        } else if (iochunk > 0) {
            iolen += (size_t)iochunk;
            continue;

        /* probably EOF */
        } else if (iochunk == 0) {
            break;
        }
    
    }

    *iolenptr = iolen;
    return FILEOPS_SUCCESS;
}


/* load a file to buffer */
int loadfile (const char *file, struct buffer **filebuf)
{
    int e = FILEOPS_FAILURE;
    int fd;

    /* check for nullpointers and reset filebuf */
    if (filebuf == NULL || file == NULL)
        return FILEOPS_NULLPOINTER;
    *filebuf = NULL;
    
    /* open file for reading */
    if ((fd = openfile(READ, file)) == -1)
        return FILEOPS_CANNOT_OPEN_READING;

    /* allocate a new buffer */ 
    if ((*filebuf = newbuffer()) == NULL)
        return FILEOPS_ALLOCATION_FAIL;

    /* create a small fixed-size buffer */
    unsigned char readbuf[ FILEOPS_CHUNKSIZE ];
    size_t readlen;
    
    /* read file in chunks */
    for (;;) {
        /* read chunk to local buffer */
        e = io (read, fd, readbuf, sizeof readbuf, &readlen);

        /* if error or EOF */
        if (e != FILEOPS_SUCCESS || readlen == 0)
            break;

        /* put local into buffer */
        if ((e = buffer_put(*filebuf, readlen, readbuf)) != BUFFER_SUCCESS)
            break;
    }

    /* cleanup */
    memzero(readbuf, sizeof readbuf);
    if (e != FILEOPS_SUCCESS) resetbuffer(*filebuf);
    close(fd);
    return e;
}

/* save a buffer to file */
int savefile (const char *file, struct buffer *filebuf)
{
    int e = FILEOPS_FAILURE;
    int fd;

    /* check for nullpointers */
    if (filebuf == NULL || file == NULL)
        return FILEOPS_NULLPOINTER;
    
    /* open for writing */
    if ((fd = openfile(WRITE, file)) == -1)
        return FILEOPS_CANNOT_OPEN_WRITING;

    /* get pointer to and size of data to write */
    size_t writelen;
    unsigned char *dataptr = buffer_dataptr(filebuf);
    size_t length = buffer_length(filebuf);

    /* try to write contents of buffer to file */
    ssize_t (*_write) (int, void *, size_t) = (ssize_t (*) (int, void *, size_t)) write;
    e = io (_write, fd, dataptr, length, &writelen);
    
    /* catch errors */
    if (e != FILEOPS_SUCCESS || writelen != length) {
        close(fd);
        unlink(file);
        return FILEOPS_INCOMPLETE_WRITE;
    }
    
    /* if successful, fsync and close */
    fsync(fd);
    close(fd);
    return e;
}
