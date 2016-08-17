#ifndef _headerguard_fileops_h_
#define _headerguard_fileops_h_

#include <sys/types.h>

enum OPENMODE { WRITE, READ };
int openfile (enum OPENMODE, const char *fn);

int readall (int fd, void *buf, size_t buf_len);

int loadfile (const char *file, void *buf, size_t buf_len);

#endif