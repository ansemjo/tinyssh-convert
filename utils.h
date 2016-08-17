#ifndef _headerguard_utils_h_
#define _headerguard_utils_h_

int strnzero (const char *str);

void ask_filename (const char *prmt, char *fn, size_t fn_len, const char *dfn);

void debugbuf (const char *name, const char *buf, size_t buf_len);

#endif