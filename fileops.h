#ifndef _headerguard_fileops_h_
#define _headerguard_fileops_h_

#include <sys/types.h>

int open_read (const char *fn);
int open_write (const char *fn);

int readall (int fd, void *buf, size_t buf_len);

int loadfile (const char *file, void *buf, size_t buf_len);

#endif