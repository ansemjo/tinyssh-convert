/*
 * This file is governed by Licenses which are listed in
 * the LICENSE file, which shall be included in all copies
 * and redistributions of this project.
 */

#ifndef _headerguard_openssh_keys_h_
#define _headerguard_openssh_keys_h_

#include <string.h>
#include <strings.h>
#include <stdlib.h>
/* for debugging printf */
#include <stdio.h>

#include "utilities.h"
#include "buffer.h"
#include "fileio.h"

/****************************************************************************************/

/* opaque sshkey struct */
struct opensshkey;

/* openssh key types */
enum openssh_keytypes {
    KEY_ED25519,
	KEY_ED25519_CERT,
    KEY_ECDSA,
    KEY_ECDSA_CERT,
    KEY_UNKNOWN,
    KEY_UNSPECIFIED,
};

/* ed25519 key constants */
#define ED25519_SECRETKEY_SIZE 64U
#define ED25519_PUBLICKEY_SIZE 32U
#define ED25519_SECRET_TINYSSH_NAME ".ed25519.sk"
#define ED25519_PUBLIC_TINYSSH_NAME "ed25519.pk"

/* statuscodes are in statuscodes.h */

/****************************************************************************************/

/* allocate and free */
struct opensshkey * newopensshkey  (int type);
               void freeopensshkey (struct opensshkey *key);

/* parsing or showing keytype */
                  int opensshkey_detect_type  (const unsigned char *keytype);
                  int opensshkey_get_type     (const struct opensshkey *key);
const unsigned char * opensshkey_get_typename (const struct opensshkey *key);

/* handle key material */
int opensshkey_set_ed25519_keys (struct opensshkey *key, unsigned char *pk, unsigned char *sk);

/* export to file */
int opensshkey_save_to_tinyssh (const struct opensshkey *key, const unsigned char *dir);

/* debugging */
void opensshkey_dump (const struct opensshkey *key);

#endif