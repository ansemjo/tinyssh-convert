/*
 * MIT License
 *
 * Copyright (c) 2016 Anton Semjonov
 * This work is derived from ssh-keygen ($OpenBSD: ssh-keygen.c,v 1.290 2016/05/02 09:36:42 djm Exp)
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


/* some openssh globals */
#include "includes.h"


/* system includes */
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

/* local includes */
#include "xmalloc.h"
#include "sshkey.h"
#include "sshbuf.h"
#include "crypto_api.h" /* for ED25519_SK_SZ */
#include "authfile.h"   /* loading private key */    
#include "pathnames.h"  /* for _PATH_HOST_ED25519_KEY_FILE */
#include "log.h"
#include "misc.h"
#include "ssherr.h"
#include "atomicio.h"

#define TINYSSH_KEYDIR "/etc/tinyssh/sshkeydir"
#define TINYSSH_ED25519_SK_NAME ".ed25519.sk"
#define TINYSSH_ED25519_PK_NAME "ed25519.pk"
#define OPENSSH_ED25519_KEY "/etc/ssh/ssh_host_ed25519_key"

extern char *__progname;
int log_level = SYSLOG_LEVEL_INFO;

/* the secretkey filename */
char keyfile[1024];
int have_keyfile = 0;

/* the destination directory */
char destination[1024];
int have_destination = 0;


/* ask for a filename */
static void ask_filename(const char *prompt, const char *initial, char *filename, size_t f_bufsize)
{
	char buf[1024];

    /* display initial prompt */
	snprintf(filename, f_bufsize, "%s", initial);
	printf("%s [%s]: ", prompt, filename);
	fflush(stdout);
	
    /* copy to filename buffer */
    if (fgets(buf, sizeof(buf), stdin) == NULL)	exit(1);
	buf[strcspn(buf, "\n")] = '\0';
	if (strcmp(buf, "") != 0) strlcpy(filename, buf, f_bufsize);
}


/* Load a private key */
static struct sshkey *load_keyfile(const char *filename)
{
	char *pass;
	struct sshkey *private;
	int r;

    /* try to load with no passphrase & early return */
	if ((r = sshkey_load_private(filename, "", &private, NULL)) == 0) return private;
	if (r != SSH_ERR_KEY_WRONG_PASSPHRASE) fatal("Load key \"%s\": %s", filename, ssh_err(r));

    /* ask for passphrase and try to load again */
	pass = read_passphrase("Enter passphrase: ", RP_ALLOW_STDIN);
	r = sshkey_load_private(filename, pass, &private, NULL);
	explicit_bzero(pass, strlen(pass));
	free(pass);
    if (r != 0)	fatal("Load key \"%s\": %s", filename, ssh_err(r));
	return private;
}

static void write_file(const char *filename, const int mode, void *data, size_t datasize)
{
    int fd, oerrno;
    /* open file descriptor */
	if ((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, mode)) < 0)
        fatal("Cannot open for writing (%s): %s\n", filename, ssh_err(SSH_ERR_SYSTEM_ERROR));

    /* write and close or unlink on failure */
	if (atomicio(vwrite, fd, data, datasize) != datasize ) {
		oerrno = errno;
		close(fd);
		unlink(filename);
		errno = oerrno;
		fatal("Error on writing file (%s): %s\n", filename, ssh_err(SSH_ERR_SYSTEM_ERROR));
	}
	close(fd);
}


static void do_convert()
{
	struct sshkey *privatekey;
	struct stat st;

    /* load the key */
	if (!have_keyfile)
        ask_filename("Enter filepath to key", OPENSSH_ED25519_KEY,
                       keyfile, sizeof(keyfile));
	if (stat(keyfile, &st) < 0) fatal("%s: %s: %s", __progname, keyfile, strerror(errno));

	privatekey = load_keyfile(keyfile);
	if (privatekey->type != KEY_ED25519) fatal("This is not an ed25519 key. Abort.");

    /* destination */
    if (!have_destination)
        ask_filename("Destination directory for tinyssh keys", TINYSSH_KEYDIR,
                       destination, sizeof(destination));


    /* write to files */
    char skey[1024]; sprintf(skey, "%s/%s", destination, TINYSSH_ED25519_SK_NAME);
    char pkey[1024]; sprintf(pkey, "%s/%s", destination, TINYSSH_ED25519_PK_NAME);

    printf("Writing %skey to: %s\n", "secret", skey);
    write_file(skey, 0600, (char *)privatekey->ed25519_sk, ED25519_SK_SZ);

    printf("Writing %skey to: %s\n", "public", pkey);
    write_file(pkey, 0644, (char *)privatekey->ed25519_pk, ED25519_PK_SZ);
    
    /* cleanup */
    sshkey_free(privatekey);
    exit(0);
    
    exit(0);
}


/* usage info */
static void usage(void)
{
	fprintf(stderr, "TODO ... :)\n");
	exit(1);
}


/* ======  MAIN  ====== */

int main(int argc, char **argv)
{
	int opt;	
    extern int optind;
	extern char *optarg;

	ssh_malloc_init();	/* must be called before any mallocs */
	sanitise_stdfd();   /* Ensure that fds 0, 1 and 2 are open or directed to /dev/null */
	__progname = ssh_get_progname(argv[0]);

	log_init(argv[0], SYSLOG_LEVEL_INFO, SYSLOG_FACILITY_USER, 1);

	while ((opt = getopt(argc, argv, "v" "f:d:")) != -1) {
		switch (opt) {
		
        /* filename */
        case 'f':
			if (strlcpy(keyfile, optarg,
			    sizeof(keyfile)) >= sizeof(keyfile))
				fatal("Private key filename too long");
			have_keyfile = 1;
			break;

        /* destination directory */
        case 'd':
			if (strlcpy(destination, optarg,
			    sizeof(destination)) >= sizeof(destination))
				fatal("Destination directory name too long");
			have_destination = 1;
			break;

        /* verbosity */
        case 'v':
			if (log_level == SYSLOG_LEVEL_INFO)
				log_level = SYSLOG_LEVEL_DEBUG1;
			else {
				if (log_level >= SYSLOG_LEVEL_DEBUG1 &&
				    log_level < SYSLOG_LEVEL_DEBUG3)
					log_level++;
			}
			break;

		case '?':
		default:
			usage();
		}
	}

	/* reinit */
	log_init(argv[0], log_level, SYSLOG_FACILITY_USER, 1);

	argv += optind;
	argc -= optind;

    
    do_convert();
    printf("Something went wrong.");
	exit(0);
}
