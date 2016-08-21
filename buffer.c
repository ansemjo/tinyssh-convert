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
    int e = BUFFER_FAILURE;

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

/* put a 32 bit unsigned number */
int buffer_put_u32 (struct buffer *buf, unsigned long value)
{
    int e = BUFFER_FAILURE;
    unsigned char *valput;
    
    /* reserve memory */
    if ((e = buffer_reserve(buf, 4, &valput)) != BUFFER_SUCCESS)
        return e;
    
    /* write the value to address */
    encode_uint32(valput, value);
    return BUFFER_SUCCESS;
}

/* put a 8 bit unsigned char */
int buffer_put_u8 (struct buffer *buf, unsigned char value)
{
    int e = BUFFER_FAILURE;
    unsigned char *valput;
    
    /* reserve memory */
    if ((e = buffer_reserve(buf, 1, &valput)) != BUFFER_SUCCESS)
        return e;
    
    /* write the value to address */
    *valput = value;
    return BUFFER_SUCCESS;
}




/* +--------------------------+ */
/* | read content from buffer | */
/* +--------------------------+ */

int buffer_add_offset (struct buffer *buf, size_t length)
{
    if (length > buffer_get_length(buf))
        return BUFFER_OFFSET_TOO_LARGE;
    
    buf->offset += length;
    return BUFFER_SUCCESS;
}

/* get a 32 bit unsigned number */
int buffer_read_u32 (struct buffer *buf, unsigned long *read)
{
    int e = BUFFER_FAILURE;
    unsigned char *tmp = buffer_get_offsetptr(buf);

    /* move offset */
    if ((e = buffer_add_offset(buf, 4)) != BUFFER_SUCCESS)
        return e;
    
    /* put number */
    if (read != NULL)
        *read = decode_uint32(tmp);

    return BUFFER_SUCCESS;
}

/* get an 8 bit unsigned number/char */
int buffer_read_u8 (struct buffer *buf, unsigned char *read)
{
    int e = BUFFER_FAILURE;
    unsigned char *tmp = buffer_get_offsetptr(buf);

    /* move offset */
    if ((e = buffer_add_offset(buf, 1)) != BUFFER_SUCCESS)
        return e;
    
    /* put number */
    if (read != NULL)
        *read = (unsigned char)*tmp;

    return BUFFER_SUCCESS;
}

/* +-----------------------+ */
/* | get info about struct | */
/* +-----------------------+ */

unsigned char *buffer_get_dataptr (struct buffer *buf)
{
    if (buf != NULL)
        return buf->data;
    return NULL;
}

unsigned char *buffer_get_offsetptr (struct buffer *buf) {
    if (buf != NULL)
        return buf->data + buf->offset;
    return NULL;
}

size_t buffer_get_datasize (struct buffer *buf)
{
    if (buf != NULL)
        return buf->size;
    return 0;
}

size_t buffer_get_allocation (struct buffer *buf)
{
    if (buf != NULL)
        return buf->allocation;
    return 0;
}

size_t buffer_get_length (struct buffer *buf)
{
    if (buf != NULL)
        return buf->size - buf->offset;
    return 0;
}


/* +-----------+ */
/* | debugging | */
/* +-----------+ */


void buffer_dump (struct buffer *buf) {
    if (buf != NULL) debugbuf("STRUCT", (unsigned char *)buf, sizeof(struct buffer));
    #include <stdio.h> /* TODO some these function shall be removed sometime */ 
    printf("%s%p\n%s%lu bytes\n%s%lu bytes\n%s+%lu = %p\n",
            "data address:   ", buf->data,
            "allocation:     ", buf->allocation,
            "data length:    ", buf->size,
            "offset:         ", buf->offset, buf->data + buf->offset);
    if (buf->data != NULL) debugbuf("BUFFER DATA", buf->data, buf->size); }
