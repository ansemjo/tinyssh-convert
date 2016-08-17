#ifndef _headerguard_fileops_h_
#define _headerguard_fileops_h_

#include <sys/types.h>

enum IO_MODE { WRITE, READ };

int loadfile (const char *file, void *buf, size_t buf_len);
int savefile (const char *file, void *buf, size_t buf_len);

#endif