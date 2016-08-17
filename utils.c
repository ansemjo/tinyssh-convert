#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "utils.h"


/* string is not null and not empty */
int strnzero (const char *str) {
    if (str != NULL)
        return strcmp(str, "") != 0;
    return 0;
}


/* ask for a filename */
void prompt (const char *prmt, char *fn, size_t fn_len, const char *dfn) {
    char buf[fn_len];
    
    /* if default path is given, copy that to fn already */
    if (strnzero(dfn))
        snprintf(fn, fn_len, "%s", dfn);

    /* display prompt */
    if (strnzero(fn))
	    printf("%s [%s]: ", prmt, fn);
    else
        printf("%s: ", fn);
	fflush(stdout);
	
    /* get filename from stdin */
    if (fgets(buf, sizeof buf, stdin) == NULL)
        exit(1); /* is this what we want? */
    
    /* find newline and replace by \0 */
	buf[strcspn(buf, "\n")] = '\0';

    /* if the buffer now has nonzero length, copy that to filename */
	if (strnzero(buf)) strncpy(fn, buf, fn_len);
}


/* print the contents of a buffer in hex */
void debugbuf (const char *name, const char *buf, size_t buf_len)
{
    printf("\n%8s   %s: %s", "address", "content of", name != NULL ? name : "UNKNOWN");
	for (size_t i = 0; i < buf_len; i++) {

		/* print address on newline */
        if (i % 16 == 0) printf("\n%08x  ", i);
        /* extra space every 8 bytes */
        if (i % 16 == 8) printf(" ");
        /* print contents in hex */
		printf(" %02x", buf[i]);
	}
    printf("\n");
}
