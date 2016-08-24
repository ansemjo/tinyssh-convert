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

struct buffer *filebuffer = NULL;

struct opensshkey *privatekey = NULL;

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
				fatale(ERR_BAD_ARGUMENT);
			have_sourcefn = 1;
            break;

        /* destination directory */
        case 'd':
			if (strncpy(destfn, optarg, sizeof destfn) == NULL)
			    fatale(ERR_BAD_ARGUMENT);
			have_destfn = 1;
			break;

        case 'h':
		case '?':
		default:
			usage();
		}
	}

    /* prompt for source if not given */
    if (!have_sourcefn &&
        (e = prompt ("Enter a source filename", sourcefn, sizeof sourcefn, "/tmp/nope.txt")) != SUCCESS)
            cleanreturn(e);

    /* load to buffer */
    if ((e = loadfile(sourcefn, &filebuffer)) != 0)
        cleanreturn(e);

    /* parse as opensshkey */
    if ((e = openssh_key_v1_parse(filebuffer, &privatekey))!= SUCCESS)
        cleanreturn(e);

    /* ask for destination */
    if (!have_destfn &&
        (e = prompt ("Enter a destination filename", destfn, sizeof destfn, "/tmp")) != SUCCESS)
            cleanreturn(e);

    /* export tinyssh keys */
    if ((e = opensshkey_save_to_tinyssh(privatekey, destfn)) != 0)
        cleanreturn(e);

    cleanup:
        freebuffer(filebuffer);
        freeopensshkey(privatekey);

    if (e != SUCCESS)
        fatale(e);

	exit(e);
}
