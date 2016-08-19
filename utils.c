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


/* ask for user input */
void prompt (const char *prmt, char *dest, size_t dest_len, const char *df_value) {
    char buf[dest_len];
    
    /* if default value is given, copy that to dest already */
    if (strnzero(df_value))
        snprintf(dest, dest_len, "%s", df_value);

    /* display prompt */
    if (strnzero(dest))
	    printf("%s [%s]: ", prmt, dest);
    else
        printf("%s: ", dest);
	fflush(stdout);
	
    /* get input from stdin */
    if (fgets(buf, sizeof buf, stdin) == NULL)
        exit(1); /* TODO is this what we want? */
    
    /* find newline and replace by \0 */
	buf[strcspn(buf, "\n")] = '\0';

    /* if the buffer now has nonzero length, copy that to dest */
	if (strnzero(buf)) strncpy(dest, buf, dest_len);
}


/* print the contents of a buffer in hex */
void debugbuf (const char *name, const unsigned char *buf, size_t buf_len)
{
    printf("\n%8s   %s: %s", "address", "content of", name != NULL ? name : "UNKNOWN");
	for (size_t i = 0; i < buf_len; i++) {

		/* print empty line every 16 lines */
        if (i % 256 == 0) printf("\n");
        /* print address on newline */
        if (i % 16 == 0) printf("\n%08x  ", i);
        /* extra space every 8 bytes */
        if (i % 16 == 8) printf(" ");
        /* print contents in hex */
		printf(" %02x", buf[i]);
	}
    printf("\n\n");
}
