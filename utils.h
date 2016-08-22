#ifndef _headerguard_utils_h_
#define _headerguard_utils_h_

#include <stdlib.h>

/* allocate zero-initialised */
#define zalloc(len) calloc(1,len)

/* volatile memset to try & avoid optmising it away */
static void * (* const volatile volatile_memset)(void *,  int, size_t) = memset;
#define memzero(ptr, size)       volatile_memset(   ptr,    0,   size)
#define memfill(ptr, size, fill) volatile_memset(   ptr, fill,   size)

/* shorthand for wiping pointers */
#define wipepointer(ptr, size) if (ptr != NULL) { memzero(ptr, size); free(ptr); ptr = NULL; }

/* early exit, e.g. if some condition failed, but do cleanup first! */
#define early_exit(status) do { e = (status); goto cleanup; } while (0)


/* check if a string is not zero and not zerolength */
int strnzero (const char *str);

/* prompt for user input */
void prompt (const char *prmt, char *fn, size_t fn_len, const char *dfn);

/* print contents of a string similar to hexdump */
void debugbuf (const char *name, const unsigned char *buf, size_t buf_len);

#endif