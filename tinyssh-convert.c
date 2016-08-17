/*
 * MIT License
 *
 * Copyright (c) 2016 Anton Semjonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Description:
 * This software converts existing ed25519 keys from OpenSSH format to the TinySSH format.
 * TinySSH is a small SSH server put into public domain [https://tinyssh.org/]. 
 */

/* system includes */
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

/* local includes */
#include "errors.h"
#include "utils.h"

/* the secretkey filename */
char keyfile[1024];
int have_keyfile = 0;

/* the destination directory */
char destination[1024];
int have_destination = 0;

/* usage info */
static void usage(void)
{
	fatal(ERR_USAGE,
        "Usage: tinyssh-convert [-f keyfile] [-d destination_dir]\n"
        "Convert an OpenSSH ed25510 privatekey file to TinySSH\n"
        "compatible format keys and save them in destination_dir.\n"
    );
}


/* ======  MAIN  ====== */

int main(int argc, char **argv)
{
	int opt;	
	extern char *optarg;

	while ((opt = getopt(argc, argv, "?hf:d:")) != -1) {
		switch (opt) {
		
        /* filename */
        case 'f':
			if (strncpy(keyfile, optarg, sizeof(keyfile)) == NULL)
				fatal(filename_too_long, "Private key filename too long");
			have_keyfile = 1;
			break;

        /* destination directory */
        case 'd':
			if (strncpy(destination, optarg, sizeof(destination)) == NULL)
			    fatal(filename_too_long, "Destination directory name too long");
			have_destination = 1;
			break;

        case 'h':
		case '?':
		default:
			usage();
		}
	}

    prompt ("Enter a filename", keyfile, sizeof keyfile, "/tmp/nope.txt");
    if (strnzero(keyfile)) printf("Keyfile is: %s\n", keyfile);
    
    debugbuf("keyfile", keyfile, strlen(keyfile));
    
    //usage();
	exit(0);
}
