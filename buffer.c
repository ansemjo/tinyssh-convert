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
    /* TODO implement 'packing', i.e. remove the offset data */

    /* is this a reasonable request? */
    if (needed_size > BUFFER_ALLOCATION_MAXIMUM)
        return BUFFER_LENGTH_OVER_MAXIMUM;

    /* do we need more allocation? */
    if (needed_size > buf->allocation) {
        /* reallocate with more mem */
        if ((newdata = realloc(buf->data, needed_size)) == NULL)
            return BUFFER_REALLOC_FAILED;
        
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
        return BUFFER_NULLPOINTER;

    /* reserve space */
    if ((e = buffer_reserve(buf, datalength, &put)) != BUFFER_SUCCESS)
        return e;

    /* copy data */
    if (memcpy(put, data, datalength) == NULL)
        return BUFFER_MEMCPY_FAIL;
    
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

/* decode a base64 string and put it into an existing buffer */
int buffer_put_decoded_base64 (struct buffer *buf, const char *base64string)
{
    int e = BUFFER_FAILURE;
    
    unsigned char *decoded;
    size_t encoded_len = strlen(base64string);
    size_t decoded_len;

    if (encoded_len == 0)
        return BUFFER_SUCCESS;

    /* allocate memory for decoded string */
    if ((decoded = malloc(encoded_len)) == NULL)
        return BUFFER_MALLOC_FAILED;

    /* try to decode string */
    if ((decoded_len = base64_decode(base64string, decoded, encoded_len)) < 0) {
        e = BUFFER_INVALID_FORMAT;
    } else { /* if successful */
        /* put decoded string into buffer */
        e = buffer_put(buf, decoded_len, decoded);
    }

    memzero(decoded, encoded_len);
    free(decoded);

    return e;
}


/* +--------------------------+ */
/* | read content from buffer | */
/* +--------------------------+ */

int buffer_add_offset (struct buffer *buf, size_t length)
{
    if (length > buffer_get_remaining(buf))
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

/* get pointer and length of next string in buffer */
int buffer_get_stringptr (const struct buffer *buf, const unsigned char **stringptr, size_t *stringlen)
{
    unsigned long length;
    const unsigned char *pointer = buffer_get_offsetptr(buf);

    /* check & clear targets */
    if (buf == NULL || stringptr == NULL || stringlen == NULL)
        return BUFFER_NULLPOINTER; 
    *stringptr = NULL;
    *stringlen = 0;

    /* check length in buffer */
    if (buffer_get_remaining(buf) < 4)
        return BUFFER_INCOMPLETE_MESSAGE;

    /* get length of string */
    length = decode_uint32(pointer);

    /* check length sanity */
    if (length > BUFFER_ALLOCATION_MAXIMUM - 4)
        return BUFFER_LENGTH_OVER_MAXIMUM;
    if (length > buffer_get_remaining(buf) - 4)
        return BUFFER_INCOMPLETE_MESSAGE;
    
    /* write results */
    *stringptr = pointer + 4;
    *stringlen = length;

    return BUFFER_SUCCESS;
}

/* read string and optionally check for continuity in respect to given nullchar */
int buffer_read_string (struct buffer *buf, unsigned char **stringptr, size_t *lengthptr, char *nullchar)
{
    const unsigned char *string, *nullcharfind;
    size_t length;
    int e = BUFFER_FAILURE;

    /* reset targets */
    if (stringptr != NULL) *stringptr = NULL;
    if (lengthptr != NULL) *lengthptr = 0;
    
    /* get pointer and length of string in buffer */
    if ((e = buffer_get_stringptr(buf, &string, &length)) != BUFFER_SUCCESS)
        return e;
    
    /* if nullchar given, check that it only appears at the end */
    if (nullchar != NULL) {
        if ( length > 0 &&
            (nullcharfind = memchr(string, *nullchar, length)) != NULL &&
             nullcharfind < string + length - 1)
                return BUFFER_INVALID_FORMAT;
    }

    /* advance offset */
    if (buffer_add_offset(buf, length + 4))
        return BUFFER_INTERNAL_ERROR;
    
    /* allocate new buffer and write string */
    if (stringptr != NULL) {
        /* allocate */
        if ((*stringptr = malloc(length + 1)) == NULL)
            return BUFFER_MALLOC_FAILED;

        /* copy string */
        if (length != 0)
            if (memcpy(*stringptr, string, length) == NULL)
                return BUFFER_MEMCPY_FAILED;

        /* terminate with nullchar */
        (*stringptr)[length] = nullchar != NULL ? *nullchar : '\0';
    }

    /* output length */
    if (lengthptr != NULL)
        *lengthptr = length;

    return BUFFER_SUCCESS;
}

/*
    Compatability with sshbuf_get_c?string:
    buffer_read_string(buf, strptr, lenptr, NULL) => sshbuf_get_string(buf, strptr, lenptr)
    buffer_read_string(buf, strptr, lenptr, '\0') => sshbuf_get_cstring(buf, strptr, lenptr)
*/


/* +---------------------------+ */
/* | create from other formats | */
/* +---------------------------+ */

/* create a new buffer from a given string */
int buffer_new_from_string (struct buffer **buf, const char *string, size_t strlen)
{
    if (buf != NULL)
        *buf = NULL;
    
    if (string == NULL)
        return BUFFER_NULLPOINTER;
    
    /* allocate new */
    if ((*buf = newbuffer()) == NULL)
        return BUFFER_ALLOCATION_FAILED;

    return buffer_put(*buf, strlen, string);    
}

/* create a new buffer from the remaining data in a given buffer */
int buffer_new_from_buffer (struct buffer **buf, const struct buffer *sourcebuf)
{
    if (buf != NULL)
        *buf = NULL;

    if (sourcebuf == NULL)
        return BUFFER_NULLPOINTER;
    
    return buffer_new_from_string(buf, buffer_get_offsetptr(sourcebuf), buffer_get_remaining(sourcebuf));
}

/* +-----------------------+ */
/* | get info about struct | */
/* +-----------------------+ */

/* returns pointer to beginning of data */
unsigned char *buffer_get_dataptr (const struct buffer *buf)
{
    if (buf != NULL)
        return buf->data;
    return NULL;
}

/* returns pointer to beginning of offset data */
unsigned char *buffer_get_offsetptr (const struct buffer *buf) {
    if (buf != NULL)
        return buf->data + buf->offset;
    return NULL;
}

/* return total size of present data */
size_t buffer_get_datasize (const struct buffer *buf)
{
    if (buf != NULL)
        return buf->size;
    return 0;
}

/* returns currently allocated mem */
size_t buffer_get_allocation (const struct buffer *buf)
{
    if (buf != NULL)
        return buf->allocation;
    return 0;
}

/* returns remaining unread / unprocessed data */
size_t buffer_get_remaining (const struct buffer *buf)
{
    if (buf != NULL)
        return buf->size - buf->offset;
    return 0;
}


/* +-----------+ */
/* | debugging | */
/* +-----------+ */


void buffer_dump (const struct buffer *buf) {
    if (buf != NULL) debugbuf("STRUCT", (unsigned char *)buf, sizeof(struct buffer));
    #include <stdio.h> /* TODO some these function shall be removed sometime */ 
    printf("%s%p\n%s%lu bytes\n%s%lu bytes\n%s+%lu = %p\n",
            "data address:   ", buf->data,
            "allocation:     ", buf->allocation,
            "data length:    ", buf->size,
            "offset:         ", buf->offset, buf->data + buf->offset);
    if (buf->data != NULL) debugbuf("BUFFER DATA", buf->data, buf->size); }
