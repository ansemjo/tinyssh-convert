#include "buffer.h"

struct buffer {
    unsigned char *data;  /* pointer to stored data */
    size_t size;          /* length of data stored */
    size_t offset;        /* offset to first available byte */
    size_t allocation;    /* length of allocated memory */

/* ASCIIFLow Structure
                                   +--A-L-L-O-C--+
                          +--------->+-D-A-T-A-+ |
    +-struct-buffer-+    /         | |    |    | |
    |               |   /          | |    |    | |
    |     *dataptr +---+         +------> |    | |
    |               |           /  | |    |    | |
    |         size +----+      /   | |____V____| |
    |               |    \    /    | |         | |
    |       offset +------\--+     | |         | |
    |               |      \       | |         | |
    |    allocated +---+    +------->|         | |
    |               |   \          | |         | |
    +---------------+    +-------->| +---------+ |
                                   |             |
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
    if ( (new->data = zalloc(new->allocation)) == NULL ) {
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
    if (buf->data != NULL) {
        memzero(buf->data, buf->allocation);
        free(buf->data);
    }

    /* clean and free struct */
    memzero(buf, sizeof *buf);
    free(buf);
}

/* reset data in buffer */ 
void resetbuffer (struct buffer *buf)
{
    if (buf == NULL) return;
    unsigned char *newdata;

    /* zero the data */
    if (buf->data != NULL)
        memzero(buf->data, buf->allocation);
    buf->offset = buf->size = 0;

    /* realloc if larger than initial */
    if (buf->allocation != BUFFER_ALLOCATION_INITIAL) {
        if ((newdata = realloc(buf->data, BUFFER_ALLOCATION_INITIAL)) != NULL) {
            buf->data = newdata;
            buf->allocation = BUFFER_ALLOCATION_INITIAL;
        }
    }

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
int buffer_reserve (struct buffer *buf, size_t request_size, unsigned char **request_ptr)
{
    size_t needed_size;
    unsigned char *newdata;

    if (request_ptr != NULL) *request_ptr = NULL;

    /* calculate next largest increment of the needed new size */
    needed_size = roundup(request_size + buf->size, BUFFER_ALLOCATION_INCREMENT);

    /* is this a reasonable request? */
    if (needed_size > BUFFER_ALLOCATION_MAXIMUM)
        return BUFFER_E_RESERVE_TOO_LARGE;

    /* do we need more allocation? */
    if (needed_size > buf->allocation) {
        /* reallocate with more mem */
        if ((newdata = realloc(buf->data, needed_size)) == NULL)
            return BUFFER_E_REALLOC_FAILED;
        
        /* set new data values in buffer */
        buf->allocation = needed_size;
        buf->data = newdata;
    }

    /* adjust 'used' size of buffer and return pointer */
    newdata = buf->data + buf->size;
    buf->size += request_size;

    if (request_ptr != NULL) *request_ptr = newdata;
    return BUFFER_SUCCESS;
}

/* put new data into buffer */
int buffer_put (struct buffer *buf, size_t datalength, const void *data)
{
    unsigned char *put;
    enum buffer_status e = BUFFER_FAILURE;

    if (data == NULL)
        return BUFFER_E_NULLPOINTER;

    /* reserve space */
    if ((e = buffer_reserve(buf, datalength, &put)) != BUFFER_SUCCESS)
        return e;

    /* copy data */
    if (memcpy(put, data, datalength) == NULL)
        return BUFFER_E_MEMCPY_FAIL;
    
    /* success */
    return BUFFER_SUCCESS;
}

/* +-----------+ */
/* | debugging | */
/* +-----------+ */

unsigned char *buffer_dataptr (struct buffer *buf) {
    if (buf != NULL) return buf->data; else return NULL; }

size_t buffer_length (struct buffer *buf) {
    if (buf != NULL) return buf->size; else return 0; }

size_t buffer_offset (struct buffer *buf) {
    if (buf != NULL) return buf->offset; else return 0; }

size_t buffer_allocation (struct buffer *buf) {
    if (buf != NULL) return buf->allocation; else return 0; }

void buffer_dump (struct buffer *buf) {
    if (buf != NULL) debugbuf("STRUCT", (unsigned char *)buf, sizeof(struct buffer));
    #include <stdio.h> /* TODO some these function shall be removed sometime */ 
    printf("%s%p\n%s%lu bytes\n%s%lu bytes\n%s+%lu = %p\n",
            "data address:   ", buf->data,
            "allocation:     ", buf->allocation,
            "data length:    ", buf->size,
            "offset:         ", buf->offset, buf->data + buf->offset);
    if (buf->data != NULL) debugbuf("BUFFER DATA", buf->data, buf->size); }
