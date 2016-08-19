#ifndef _headerguard_utils_h_
#define _headerguard_utils_h_

int strnzero (const char *str);

void prompt (const char *prmt, char *fn, size_t fn_len, const char *dfn);

void debugbuf (const char *name, const unsigned char *buf, size_t buf_len);

#endif