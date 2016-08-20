#include "fileops.h"

/* direct io on a file descriptor from/to a buffer */
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
    if ((fd = openreading(file)) == -1)
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
        e = io (ioread, fd, readbuf, sizeof readbuf, &readlen);

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
    if ((fd = openwriting(file)) == -1)
        return FILEOPS_CANNOT_OPEN_WRITING;

    /* get pointer to and size of data to write */
    size_t writelen;
    unsigned char *dataptr = buffer_get_dataptr(filebuf);
    size_t length = buffer_get_size(filebuf);

    /* try to write contents of buffer to file */
    e = io (iowrite, fd, dataptr, length, &writelen);
    
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
