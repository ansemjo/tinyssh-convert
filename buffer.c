#include "buffer.h"

struct buffer_ {
    unsigned char *dataptr; /* pointer to stored data */
    size_t offset;          /* offset to first available byte */
    size_t length;          /* length of data stored */
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

void bufferfree (struct buffer *buf)
{
    if (buf == NULL) return;

    if (buf->dataptr != NULL) {
        memzero(buf->dataptr, buf->allocation);
        free(buf->dataptr);
    }

    memzero(buf, sizeof *buf);
    free(buf);
}


/*
fill with random data and then sum it
to avoid any chance of optimization ?

global scope ..
static int accum=0;

for(.)
key[i]=key[rand()]&0x1+rand();

for(.)
accum+=key[rand()];
}
*/