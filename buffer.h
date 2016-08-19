#ifndef _headerguard_buffer_h_
#define _headerguard_buffer_h_

#include <sys/types.h>
#include <sys/param.h>
#include <string.h>
#include <stdlib.h>

#include "utils.h"

/* allocate zero-initialised */
#define zalloc(len) calloc(1,len)

/* volatile memset to try & avoid optmising it away */
static void * (* const volatile volatile_memset)(void *,  int, size_t) = memset;
#define memzero(ptr, size)       volatile_memset(   ptr,    0,   size)
#define memfill(ptr, size, fill) volatile_memset(   ptr, fill,   size)


/* size constraints */
#define BUFFER_ALLOCATION_INITIAL                512  /*   512 B   */
#define BUFFER_ALLOCATION_INCREMENT             1024  /*  1024 B   */
#define BUFFER_ALLOCATION_MAXIMUM       64*1024*1024  /*    64 MiB */

/* status codes */
typedef enum {
    BUFFER_SUCCESS = 0,
    BUFFER_FAILURE = 1,

    /* allocating memory */
    BUFFER_E_RESERVE_TOO_LARGE,
    BUFFER_E_REALLOC_FAILED,

} BUFFER_STATUSCODE;

/* opaque struct */
struct buffer;

/* allocate and free buffers */
struct buffer * newbuffer   ();
           void freebuffer  (struct buffer *buf);

/* put new data in */
            int buffer_reserve  (struct buffer *buf, size_t request_size, unsigned char **request_ptr);
            int buffer_put      (struct buffer *buf, size_t datalen, const void *data);

/* debugging */
unsigned char * buffer_dataptr    (struct buffer *buf);
         size_t buffer_length     (struct buffer *buf);
         size_t buffer_offset     (struct buffer *buf);
         size_t buffer_allocation (struct buffer *buf);
           void buffer_dump       (struct buffer *buf);

#endif