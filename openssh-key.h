#ifndef _headerguard_openssh_keys_h_
#define _headerguard_openssh_keys_h_

#include <string.h>
#include <strings.h>
#include <stdlib.h>
/* for debugging printf */
#include <stdio.h>

#include "utils.h"

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

/* ed25519 key sizes */
#define ED25519_SECRETKEY_SIZE 64U
#define ED25519_PUBLICKEY_SIZE 32U

/* error codes */
enum openssh_key_status {
    OPENSSH_KEY_SUCCESS =   0,
    OPENSSH_KEY_FAILURE = -50,

    OPENSSH_KEY_NULLPOINTER,
    OPENSSH_KEY_MISSING_ARGUMENTS,
    OPENSSH_KEY_INCOMPATIBLE,
};

/* allocate and free */
struct opensshkey * newopensshkey  (int type);
               void freeopensshkey (struct opensshkey *key);

/* parsing or showing keytype */
                  int opensshkey_detect_type  (const unsigned char *keytype);
                  int opensshkey_get_type     (const struct opensshkey *key);
const unsigned char * opensshkey_get_typename (const struct opensshkey *key);

/* set key values */
int opensshkey_set_ed25519_keys (struct opensshkey *key, unsigned char *pk, unsigned char *sk);


/* debugging */
void opensshkey_dump (const struct opensshkey *key);

#endif