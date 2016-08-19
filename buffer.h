#ifndef _headerguard_buffer_h_
#define _headerguard_buffer_h_

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#include "defines.h"

/* allocate zero-initialised */
#define zalloc(len) calloc(1,len)

/* volatile memset to try & avoid optmising it away */
static void * (* const volatile volatile_memset)(void *,  int, size_t) = memset;
#define memzero(ptr, size)       volatile_memset(   ptr,    0,   size)
#define memfill(ptr, size, fill) volatile_memset(   ptr, fill,   size)

/* size constraints */
#define BUFFER_ALLOCATION_INITIAL    512*Bytes
#define BUFFER_ALLOCATION_STEP       512*Bytes
#define BUFFER_ALLOCATION_MAXIMUM   64*MiBytes

/* opaque struct */
//struct buffer;

struct buffer {
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


/* create & free buffers */
struct buffer *newbuffer ();
void bufferfree (struct buffer *ptr);

#endif