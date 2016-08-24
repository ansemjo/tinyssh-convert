#include "buffer.h"

/* roundup for the reserve function (from sys/types.h) */
# define roundup(x, y)  ((((x) + ((y) - 1)) / (y)) * (y))

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
    new->allocation = BUFFER_ALLOCATION_INCREMENT;
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
    if (buf->allocation != BUFFER_ALLOCATION_INCREMENT) {
        if ((newdata = realloc(buf->data, BUFFER_ALLOCATION_INCREMENT)) != NULL) {
            buf->data = newdata;
            buf->allocation = BUFFER_ALLOCATION_INCREMENT;
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

/* reserve space for new data. if request_ptr given, increase 'used' space and return pointer */
int buffer_reserve (struct buffer *buf, size_t request_size, unsigned char **request_ptr)
{
    size_t needed_size;
    unsigned char *newdata;

    if (buf == NULL)
        return ERR_NULLPTR;

    if (request_ptr != NULL)
        *request_ptr = NULL;

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

    /* adjust 'used' size of buffer and return pointer if request_ptr given */
    /* otherwise this is purely size-checking and realloc'ing */
    if (request_ptr != NULL) {
        newdata = buf->data + buf->size;
        buf->size += request_size;
        *request_ptr = newdata;
    }
    return SUCCESS;
}

/* put new data into buffer */
int buffer_put (struct buffer *buf, const void *data, size_t datalength)
{
    unsigned char *put;
    int e = FAILURE;

    if (data == NULL || buf == NULL)
        return ERR_NULLPTR;

    /* reserve space */
    if ((e = buffer_reserve(buf, datalength, &put)) != SUCCESS)
        return e;

    /* copy data */
    if (memcpy(put, data, datalength) == NULL)
        return BUFFER_MEMCPY_FAIL;
    
    /* success */
    return SUCCESS;
}

/* put a 32 bit unsigned number */
int buffer_put_u32 (struct buffer *buf, unsigned long value)
{
    int e = FAILURE;
    unsigned char *valput;
    
    /* reserve memory */
    if ((e = buffer_reserve(buf, 4, &valput)) != SUCCESS)
        return e;
    
    /* write the value to address */
    encode_uint32(valput, value);
    return SUCCESS;
}

/* put a 8 bit unsigned char */
int buffer_put_u8 (struct buffer *buf, unsigned char value)
{
    int e = FAILURE;
    unsigned char *valput;
    
    /* reserve memory */
    if ((e = buffer_reserve(buf, 1, &valput)) != SUCCESS)
        return e;
    
    /* write the value to address */
    *valput = value;
    return SUCCESS;
}

/* decode a base64 string and put it into an existing buffer */
int buffer_put_decoded_base64 (struct buffer *buf, const char *base64string)
{
    int e = FAILURE;
    
    unsigned char *decoded;
    size_t encoded_len = strlen(base64string);
    size_t decoded_len;

    if (encoded_len == 0)
        return SUCCESS;

    /* allocate memory for decoded data */
    if ((decoded = malloc(encoded_len)) == NULL)
        return BUFFER_MALLOC_FAILED;

    /* try to decode string */
    if ((decoded_len = base64_decode(base64string, decoded, encoded_len)) < 0) {
        e = BUFFER_INVALID_FORMAT;
    } else { /* if successful */
        /* put decoded data into buffer */
        e = buffer_put(buf, decoded, decoded_len);
    }

    memzero(decoded, encoded_len);
    free(decoded);

    return e;
}

/* put a string of data with prefixed u32 length */
int buffer_put_data (struct buffer *buf, void *data, size_t length)
{
    int e = FAILURE;

    if (buf == NULL || data == NULL)
        return ERR_NULLPTR;

    /* check length requirement / reserve space */
    if ((e = buffer_reserve(buf, length + 4, NULL)) != SUCCESS)
        return e;
    
    /* put length and string */
    if ((e = buffer_put_u32(buf, length)) != SUCCESS ||
        (e = buffer_put(buf, data, length)) != SUCCESS)
        return e;

    return e;
}

/* put a character string */
int buffer_put_string (struct buffer *buf, unsigned char *string)
{
    if (buf == NULL || string == NULL)
        return ERR_NULLPTR;

    /* get stringlength */
    size_t length = strlen(string);

    /* put string into buffer */
    return buffer_put_data(buf, string, length);
}


/* +--------------------------+ */
/* | read content from buffer | */
/* +--------------------------+ */

int buffer_add_offset (struct buffer *buf, size_t length)
{
    if (length > buffer_get_remaining(buf))
        return BUFFER_OFFSET_TOO_LARGE;
    
    buf->offset += length;
    return SUCCESS;
}

/* get a 32 bit unsigned number */
int buffer_read_u32 (struct buffer *buf, unsigned long *read)
{
    int e = FAILURE;
    unsigned char *tmp = buffer_get_offsetptr(buf);

    /* move offset */
    if ((e = buffer_add_offset(buf, 4)) != SUCCESS)
        return e;
    
    /* put number */
    if (read != NULL)
        *read = decode_uint32(tmp);

    return SUCCESS;
}

/* get an 8 bit unsigned number/char */
int buffer_read_u8 (struct buffer *buf, unsigned char *read)
{
    int e = FAILURE;
    unsigned char *tmp = buffer_get_offsetptr(buf);

    /* move offset */
    if ((e = buffer_add_offset(buf, 1)) != SUCCESS)
        return e;
    
    /* put number */
    if (read != NULL)
        *read = (unsigned char)*tmp;

    return SUCCESS;
}

/* get pointer and length of next string in buffer */
int buffer_get_stringptr (const struct buffer *buf, const unsigned char **stringptr, size_t *stringlen)
{
    unsigned long length;
    const unsigned char *pointer = buffer_get_offsetptr(buf);

    /* check & clear targets */
    if (buf == NULL || stringptr == NULL || stringlen == NULL)
        return ERR_NULLPTR; 
    *stringptr = NULL;
    *stringlen = 0;

    /* check length in buffer */
    size_t remain = 0;
    if ((remain = buffer_get_remaining(buf)) < 4) {
        /* end of buffer */
        if (remain == 0)
            return BUFFER_END_OF_BUF;
        /* or incomplete message */
        return BUFFER_INCOMPLETE_MESSAGE;
    }

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

    return SUCCESS;
}

/* inline nullcharcheck */
static inline int buffer_nullcharcheck (const unsigned char *data, size_t length, const char *nullchar) {
    unsigned char *find;
    return ( length > 0 && (find = memchr(data, *nullchar, length)) != NULL && find < data + length - 1);
}

/* read string and optionally check for continuity in respect to given nullchar */
int buffer_read_string (struct buffer *buf, unsigned char **stringptr, size_t *lengthptr, char *nullchar)
{
    const unsigned char *string, *nullcharfind;
    size_t length;
    int e = FAILURE;

    /* reset targets */
    if (stringptr != NULL) *stringptr = NULL;
    if (lengthptr != NULL) *lengthptr = 0;
    
    /* get pointer and length of string in buffer */
    if ((e = buffer_get_stringptr(buf, &string, &length)) != SUCCESS)
        return e;
    
    /* if nullchar given, check that it does not appear within */
    if (nullchar != NULL && !buffer_nullcharcheck(string, length, nullchar))
        return BUFFER_INVALID_FORMAT;

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
                return BUFFER_MEMCPY_FAIL;

        /* terminate with nullchar */
        (*stringptr)[length] = nullchar != NULL ? *nullchar : '\0';
    }

    /* output length */
    if (lengthptr != NULL)
        *lengthptr = length;

    return SUCCESS;
}

/*
    Compatability with sshbuf_get_c?string:
    buffer_read_string(buf, strptr, lenptr, NULL) => sshbuf_get_string(buf, strptr, lenptr)
    buffer_read_string(buf, strptr, lenptr, '\0') => sshbuf_get_cstring(buf, strptr, lenptr)
*/


/* +---------------------------+ */
/* | create from other formats | */
/* +---------------------------+ */

/* create a new buffer from a given datastring */
int buffer_new_from_data (struct buffer **newbuf, const char *data, size_t datalen)
{
    int e = FAILURE;

    if (newbuf != NULL)
        *newbuf = NULL;
    
    if (data == NULL)
        return ERR_NULLPTR;
    
    /* allocate new */
    if ((*newbuf = newbuffer()) == NULL)
        return BUFFER_ALLOCATION_FAILED;

    /* put in data and free on failure */
    if ((e = buffer_put(*newbuf, data, datalen)) != SUCCESS)
        freebuffer(*newbuf);
    return e;    
}

/* create a new buffer from the remaining data in a given buffer */
int buffer_new_from_buffer (struct buffer **newbuf, const struct buffer *sourcebuf)
{
    if (sourcebuf == NULL)
        return ERR_NULLPTR;
    
    return buffer_new_from_data(newbuf, buffer_get_offsetptr(sourcebuf), buffer_get_remaining(sourcebuf));
}

/* create a new buffer from a concatenation of all datastrings in a buffer */
int buffer_new_concat_data (struct buffer **newbuf, struct buffer *sourcebuf, const char *nullchar)
{
    int e = FAILURE;

    /* create new buffer with four empty bytes at the beginning */
    if ((e = buffer_new_from_data(newbuf, "\0\0\0\0", 4)) != SUCCESS)
        return e;

    /* variables to concatenate data */
    const unsigned char *concatptr;
    size_t concatlen = 0, accumlength = 0;

    /* loop to collect all datastrings, assuming there are only datastrings */
    while (e != BUFFER_END_OF_BUF) {

        /* read next pointer, switch by status */
        switch (e = buffer_get_stringptr(sourcebuf, &concatptr, &concatlen)) {

            /* there is another string */
            case SUCCESS:
                /* if nullchar given, check that it does not appear within */
                if (nullchar != NULL && !buffer_nullcharcheck(concatptr, concatlen, nullchar))
                    cleanreturn(BUFFER_INVALID_FORMAT);
                
                /* advance offset */
                if (buffer_add_offset(sourcebuf, concatlen + 4))
                    cleanreturn(BUFFER_INTERNAL_ERROR);

                /* put data into new buffer */
                if ((e = buffer_put(*newbuf, concatptr, concatlen)) != SUCCESS)
                    cleanreturn(e);

                /* increment accumulated length */
                accumlength += concatlen;

                /* great, next string */
                continue;

            /* end reached cleanly */
            case BUFFER_END_OF_BUF:
                break;

            /* any other error */
            default:
                cleanreturn(e);
                break;
        }
    }
    e = SUCCESS;
    /* on clean end of buffer, write accumulated length to first 4 bytes */
    encode_uint32(buffer_get_dataptr(*newbuf), accumlength);

    cleanup:
        if (e != SUCCESS) {
            freebuffer(*newbuf);
            *newbuf = NULL;
        }
    
    return e;
}

/* create a new buffer from a concatenation of all charstrings in a buffer */
int buffer_new_concat_strings (struct buffer **newbuf, struct buffer *sourcebuf)
{
    return buffer_new_concat_data(newbuf, sourcebuf, '\0');
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
    if (buf != NULL) { 
        debugbuf("STRUCT", (unsigned char *)buf, sizeof(struct buffer));
        #include <stdio.h> /* TODO some these function shall be removed sometime */ 
        printf("%s%p\n%s%lu bytes\n%s%lu bytes\n%s+%lu = %p\n",
                "data address:   ", buf->data,
                "allocation:     ", buf->allocation,
                "data length:    ", buf->size,
                "offset:         ", buf->offset, buf->data + buf->offset);
        if (buf->data != NULL) debugbuf("BUFFER DATA", buf->data, buf->size);
    }
}
