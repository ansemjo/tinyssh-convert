/*
 * This file is governed by Licenses which are listed in
 * the LICENSE file, which shall be included in all copies
 * and redistributions of this project.
 *
 * Some concepts and original work is derived from these files:
 *  - $OpenBSD: atomicio.c,v 1.28 2016/07/27 23:18:12 djm Exp $
 *
 * Copyright (c) 2006 Damien Miller. All rights reserved.
 * Copyright (c) 2005 Anil Madhavapeddy. All rights reserved.
 * Copyright (c) 1995,1999 Theo de Raadt.  All rights reserved.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fileio.h"

/* open file descriptors */
extern int openwriting (const char *file) {
    return open(file, O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC, 0644);
}
extern int openreading (const char *file) {
    return open(file, O_RDONLY | O_CLOEXEC);
}


/* direct io on a file descriptor from/to a buffer */
extern int io (ssize_t (*rw) (int, void *, size_t), int fd, void *data, size_t datalen, size_t *iolenptr)
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
            return FILEIO_IOERROR;
        
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
    return SUCCESS;
}


/* load a file to buffer */
extern int loadfile (const char *file, struct buffer **filebuf)
{
    int e = FAILURE;
    int fd;

    /* check for nullpointers and reset filebuf */
    if (filebuf == NULL || file == NULL)
        return ERR_NULLPTR;
    *filebuf = NULL;
    
    /* open file for reading */
    if ((fd = openreading(file)) == -1)
        return FILEIO_CANNOT_OPEN_READING;

    /* allocate a new buffer */ 
    if ((*filebuf = newbuffer()) == NULL)
        return BUFFER_ALLOCATION_FAILED;

    /* create a small fixed-size buffer */
    unsigned char readbuf[ FILEIO_CHUNKSIZE ];
    size_t readlen;
    
    /* read file in chunks */
    for (;;) {
        /* read chunk to local buffer */
        e = io (read, fd, readbuf, sizeof readbuf, &readlen);

        /* if error or EOF */
        if (e != SUCCESS || readlen == 0)
            break;

        /* put local into buffer */
        if ((e = buffer_put(*filebuf, readbuf, readlen)) != SUCCESS)
            break;
    }

    /* cleanup */
    memzero(readbuf, sizeof readbuf);
    if (e != SUCCESS) resetbuffer(*filebuf);
    close(fd);
    return e;
}

/* save a string as data to a file */
extern int savestring (const char *file, unsigned char *string, size_t stringlen)
{
    int e = FAILURE;
    int fd;

    /* check for nullpointers */
    if (string == NULL || file == NULL)
        return ERR_NULLPTR;
    
    /* open for writing */
    if ((fd = openwriting(file)) == -1)
        return FILEIO_CANNOT_OPEN_WRITING;

    /* variable to check written bytes */
    size_t writelen;

    /* try to write contents of buffer to file */
    e = io ((ssize_t (*) (int, void *, size_t))write, fd, string, stringlen, &writelen);
    
    /* catch errors */
    if (e != SUCCESS || writelen != stringlen) {
        close(fd);
        unlink(file);
        return FILEIO_INCOMPLETE_WRITE;
    }
    
    /* if successful, fsync and close */
    fsync(fd);
    close(fd);
    return e;
}

/* save a buffer to file */
extern int savefile (const char *file, struct buffer *filebuf)
{
    /* check for nullpointers */
    if (filebuf == NULL || file == NULL)
        return ERR_NULLPTR;
    
    /* get pointer to and size of data to write */
    unsigned char *dataptr = buffer_get_dataptr(filebuf);
    size_t length = buffer_get_datasize(filebuf);

    /* try to write contents of buffer to file */
    return savestring (file, dataptr, length);
}
