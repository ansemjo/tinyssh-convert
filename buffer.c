#include <sys/param.h>

#include "buffer.h"
#include "utils.h"

struct buffer {
    unsigned char *dataptr; /* pointer to stored data */
    size_t size;            /* length of data stored */
    size_t offset;          /* offset to first available byte */
    size_t allocation;      /* length of allocated memory */

/* ASCIIFLow Structure
                                   +--A-L-L-O-C--+
                                   |             |
                       +------------>+-D-A-T-A-+ |
    +-struct-buffer-+  |           | |    |    | |
    |               |  |           | |    |    | |
    |     *dataptr +---+  +-------------> |    | |
    |               |     |        | |    |    | |
    |       offset +------+        | |----v----| |
    |               |              | |         | |
    |       length +---------------->|         | |
    |               |              | |         | |
    |    allocated +-----+         | |         | |
    |               |    |         | |         | |
    +---------------+    +-------->| |         | |
                                   | |         | |
                                   | +---------+ |
                                   |             |
                                   |             |
                                   +-------------+      */
};

/* +-------------------+ */
/* | allocate and free | */
/* +-------------------+ */

/* allocate a new buffer, return pointer */
struct buffer *newbuffer ()
{
    struct buffer *new;

    /* zero-allocate struct */
    if ( (new = zalloc(sizeof *new)) == NULL )
        return NULL;
    
    /* set initial allocation size & allocate data */
    new->allocation = BUFFER_ALLOCATION_INITIAL;
    if ( (new->dataptr = zalloc(new->allocation)) == NULL ) {
        free(new);
        return NULL;
    }

    /* return pointer to allocated struct */
    return new;
}

/* free a buffer by filling with zeroes */
void freebuffer (struct buffer *buf)
{
    if (buf == NULL) return;

    /* clean and free data */
    if (buf->dataptr != NULL) {
        memzero(buf->dataptr, buf->allocation);
        free(buf->dataptr);
    }

    /* clean and free struct */
    memzero(buf, sizeof *buf);
    free(buf);
}

/* TODO, maybe? */
void freebuffer_paranoid (struct buffer *buf)
{ /*
    fill with random data and then sum it
    to avoid any chance of optimization ?

    global scope ..
    static int accum=0;

    for(.)
    key[i]=key[rand()]&0x1+rand();

    for(.)
    accum+=key[rand()];
    }
*/ return; }


/* +-----------------+ */
/* | put new data in | */
/* +-----------------+ */

/* reserve space for new data and return pointer */
unsigned char *buffer_reserve (struct buffer *buf, size_t request)
{
    size_t needed;
    unsigned char *dataptr;

    /* calculate next increment of needed new size */
    needed = roundup(request + buf->size, BUFFER_ALLOCATION_INCREMENT);

    /* is this a reasonable request? */
    if (request > BUFFER_ALLOCATION_MAXIMUM || needed > BUFFER_ALLOCATION_MAXIMUM)
        return NULL;

    /* do we need more allocation? */
    if (needed > buf->allocation) {
        /* reallocate */
        if ((dataptr = realloc(buf->dataptr, needed)) == NULL)
            return NULL;
        
        /* set new values in buffer */
        buf->allocation = needed;
        buf->dataptr = dataptr;
    }

    /* adjust 'used' size of buffer and return pointer */
    dataptr = buf->dataptr + buf->size;
    buf->size += request;

    return dataptr;
}

/* +-----------+ */
/* | debugging | */
/* +-----------+ */

unsigned char *buffer_dataptr (struct buffer *buf) {
    if (buf != NULL) return buf->dataptr; else return NULL; }

size_t buffer_length (struct buffer *buf) {
    if (buf != NULL) return buf->size; else return 0; }

size_t buffer_offset (struct buffer *buf) {
    if (buf != NULL) return buf->offset; else return 0; }

size_t buffer_allocation (struct buffer *buf) {
    if (buf != NULL) return buf->allocation; else return 0; }

void buffer_dump (struct buffer *buf) {
    if (buf != NULL) debugbuf("STRUCT", (unsigned char *)buf, sizeof(struct buffer));
    if (buf->dataptr != NULL) debugbuf("BUFFER DATA", buf->dataptr, buf->allocation); }
