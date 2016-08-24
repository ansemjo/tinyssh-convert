#ifndef _headerguard_buffer_h_
#define _headerguard_buffer_h_

#include <string.h>
#include <stdlib.h>

#include "errors.h"
#include "utilities.h"
#include "base64.h"

/****************************************************************************************/

/* size constraints */
#define BUFFER_ALLOCATION_INITIAL                512  /*   512 B */
#define BUFFER_ALLOCATION_INCREMENT             2048  /*   2 KiB */
#define BUFFER_ALLOCATION_MAXIMUM       64*1024*1024  /*  64 MiB */

/* statuscodes are defined in statuscodes.h */

/* opaque struct */
struct buffer;

/****************************************************************************************/

/* allocate and free buffers */
struct buffer * newbuffer   ();
           void freebuffer  (struct buffer *buf);
           void resetbuffer (struct buffer *buf);

/* put data into buffer */
int buffer_reserve      (struct buffer *buf, size_t request_size, unsigned char **request_ptr);
int buffer_put          (struct buffer *buf, const void *data, size_t datalength);
int buffer_put_u32      (struct buffer *buf, unsigned long value);
int buffer_put_u8       (struct buffer *buf, unsigned char value);
 #define buffer_put_char buffer_put_u8

/* convert from other formats */
int buffer_put_decoded_base64 (struct buffer *buf, const char *base64string);

/* read data from buffer */
int buffer_add_offset    (struct buffer *buf, size_t length);
int buffer_read_u32      (struct buffer *buf, unsigned long *read);
int buffer_read_u8       (struct buffer *buf, unsigned char *read);
int buffer_get_stringptr (const struct buffer *buf, const unsigned char **stringptr, size_t *stringlen);
int buffer_read_string   (struct buffer *buf, unsigned char **stringptr, size_t *lengthptr, char *nullchar);

/* create new from some other data */
int buffer_new_from_string (struct buffer **buf, const char *string, size_t strlen);
int buffer_new_from_buffer (struct buffer **buf, const struct buffer *sourcebuf);

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