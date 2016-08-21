#ifndef _headerguard_buffer_h_
#define _headerguard_buffer_h_

#include <sys/types.h>
#include <sys/param.h>
#include <string.h>
#include <stdlib.h>

#include "utils.h"
#include "base64.h"

/* allocate zero-initialised */
#define zalloc(len) calloc(1,len)

/* volatile memset to try & avoid optmising it away */
static void * (* const volatile volatile_memset)(void *,  int, size_t) = memset;
#define memzero(ptr, size)       volatile_memset(   ptr,    0,   size)
#define memfill(ptr, size, fill) volatile_memset(   ptr, fill,   size)


/* size constraints */
#define BUFFER_ALLOCATION_INITIAL                512  /*   512 B */
#define BUFFER_ALLOCATION_INCREMENT             2048  /*   2 KiB */
#define BUFFER_ALLOCATION_MAXIMUM       64*1024*1024  /*  64 MiB */

/* status codes */
enum buffer_status {
    BUFFER_SUCCESS = 0,
    BUFFER_FAILURE = 1,
 /* errors */
    BUFFER_NULLPOINTER,
    BUFFER_INTERNAL_ERROR,
    BUFFER_LENGTH_OVER_MAXIMUM,
    BUFFER_MEMCPY_FAIL,
    BUFFER_ALLOCATION_FAILED,
    BUFFER_MALLOC_FAILED,
    BUFFER_REALLOC_FAILED,
    BUFFER_MEMCPY_FAILED,
    BUFFER_OFFSET_TOO_LARGE,
    BUFFER_INCOMPLETE_MESSAGE,
    BUFFER_INVALID_FORMAT,
};

/* opaque struct */
struct buffer;

/* allocate and free buffers */
struct buffer * newbuffer   ();
           void freebuffer  (struct buffer *buf);
           void resetbuffer (struct buffer *buf);

/* put data into buffer */
int buffer_reserve  (struct buffer *buf, size_t request_size, unsigned char **request_ptr);
int buffer_put      (struct buffer *buf, size_t datalength, const void *data);
int buffer_put_u32  (struct buffer *buf, unsigned long value);
int buffer_put_u8   (struct buffer *buf, unsigned char value);
#define buffer_put_char buffer_put_u8

/* read data from buffer */
int buffer_add_offset    (struct buffer *buf, size_t length);
int buffer_read_u32      (struct buffer *buf, unsigned long *read);
int buffer_read_u8       (struct buffer *buf, unsigned char *read);
int buffer_get_stringptr (const struct buffer *buf, const unsigned char **stringptr, size_t *stringlen);
int buffer_read_string   (struct buffer *buf, unsigned char **stringptr, size_t *lengthptr, char *nullchar);

/* convert from other formats */
int buffer_decode_from_base64 (struct buffer *buf, const char *base64string);

/* attribute getters */
unsigned char * buffer_get_dataptr      (const struct buffer *buf);
unsigned char * buffer_get_offsetptr    (const struct buffer *buf);
         size_t buffer_get_datasize     (const struct buffer *buf);
         size_t buffer_get_allocation   (const struct buffer *buf);
         size_t buffer_get_remaining    (const struct buffer *buf);

/* debugging */
void buffer_dump (const struct buffer *buf);

/* Macros for decoding/encoding integers */
#define decode_uint32(addr) \
	(((unsigned long)(((const unsigned char *)(addr))[0]) << 24) | \
	 ((unsigned long)(((const unsigned char *)(addr))[1]) << 16) | \
	 ((unsigned long)(((const unsigned char *)(addr))[2]) <<  8) | \
	  (unsigned long)(((const unsigned char *)(addr))[3]))

#define encode_uint32(addr, value) \
	do { \
		const unsigned long __value = (value); \
		((unsigned char *)(addr))[0] = (__value >> 24) & 0xff; \
		((unsigned char *)(addr))[1] = (__value >> 16) & 0xff; \
		((unsigned char *)(addr))[2] = (__value >>  8) & 0xff; \
		((unsigned char *)(addr))[3] =  __value        & 0xff; \
	} while (0)


#endif