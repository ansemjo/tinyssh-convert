/*
 * This file is governed by "the MIT License", which should be
 * included in a LICENSE file in all copies of this project.
 *
 * Description:
 * This software converts existing ed25519 keys from OpenSSH format 
 * (openssh-key-v1) to the simple TinySSH binary format.
 */

 #define USAGE_MESSAGE \
    "Usage: tinyssh-convert [-f keyfile] [-d destination_dir]\n" \
    "Convert an OpenSSH ed25510 privatekey file to TinySSH\n" \
    "compatible format keys and save them in destination_dir."

/* system includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

/* local includes */
#include "errors.h"
#include "utilities.h"
#include "fileio.h"
#include "buffer.h"
#include "openssh-parse.h"
#include "openssh-key.h"

/* the secretkey filename */
char sourcefn[1024];
int have_sourcefn = 0;

/* the destination directory */
char destfn[1024];
int have_destfn = 0;

/* ======  MAIN  ====== */

int main(int argc, char **argv)
{
	int opt, e;
	extern char *optarg;

    /* parse arguments */
	while ((opt = getopt(argc, argv, "?hf:d:")) != -1) {
		switch (opt) {

        /* filename */
        case 'f':
			if (strncpy(sourcefn, optarg, sizeof sourcefn) == NULL)
				fatal(ERR_BAD_ARGUMENT, "privatekey filename too long");
			have_sourcefn = 1;
            break;

        /* destination directory */
        case 'd':
			if (strncpy(destfn, optarg, sizeof destfn) == NULL)
			    fatal(ERR_BAD_ARGUMENT, "destination directory name too long");
			have_destfn = 1;
			break;

        case 'h':
		case '?':
		default:
			usage();
		}
	}

    /* prompt for source if not given */
    if (!have_sourcefn)
        prompt ("Enter a source filename", sourcefn, sizeof sourcefn, "/tmp/nope.txt");

    /* how to successfully put hex data into new buf */
    struct buffer *hexbuf;
    if (buffer_new_from_data(&hexbuf, "\0\0\0\0", 4) == SUCCESS)
        buffer_dump(hexbuf);


    /* test buffers for strings */
    struct buffer *b = newbuffer();
    buffer_put_string(b, "12345678");
    buffer_put_string(b, "ABCDEFGH");
    buffer_put_string(b, "\xFF");
    buffer_put_string(b, USAGE_MESSAGE);

    struct buffer *concat;
    if (buffer_new_concat_strings(&concat, b) == SUCCESS)
        buffer_dump(concat);

    size_t slen = 0;
    unsigned char *str;
    if ((e = buffer_read_string(concat, &str, &slen, '\0')) == SUCCESS)
        printf("read %lu bytes string from buffer: %s\n", slen, str);

//    sshbuf_put_cstring

    fatale(SUCCESS);

    /* load to buffer */
    struct buffer *filebuffer;
    if ((e = loadfile(sourcefn, &filebuffer)) != 0)
        printf("loadfile error: %d", e);

    struct opensshkey *privatekey = NULL;
    if ((e = openssh_key_v1_parse(filebuffer, &privatekey))!= SUCCESS)
        printf("PARSING FAILED WITH ERRORCODE %d\n", e);

    /* ask for destination */
    if (!have_destfn)
        prompt ("Enter a destination filename", destfn, sizeof destfn, "/tmp");

    if ((e = opensshkey_save_to_tinyssh(privatekey, destfn)) != 0)
        printf("key export status: %d", e);

    freebuffer(filebuffer);
    freeopensshkey(privatekey);

    /*
    struct buffer *newbuff;
    newbuff = newbuffer();

    printf("allocated new buffer\n");
    buffer_dump(newbuff);

    unsigned char *newdata;
    int length[]  = {   8,   7,   16,   32};
    int pattern[] = {0xAE, 0xFF, 0x13, 0xF0};

    for (int i = 0; i < sizeof *length; i++) {
        if (buffer_reserve(newbuff, length[i], &newdata) == SUCCESS)
            memset(newdata, pattern[i], length[i]);
    }

    printf("enlarged buffer with some data\n");
    buffer_dump(newbuff);

    freebuffer(newbuff);
    */

	exit(0);
}
