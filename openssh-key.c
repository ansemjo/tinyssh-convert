#include "openssh-key.h"

/* opaque EC_KEY + ECDSA NIDs to make it compile for now .. */
typedef struct EC_KEY EC_KEY;
enum ecdsa_nids { NID_X9_62_prime256v1, NID_secp384r1, NID_secp521r1 };

/* +-------------------+ */
/* | structs and types | */
/* +-------------------+ */

/* openssh key struct */
struct opensshkey {
    int	 type;
	/* ecdsa nist curves */
	int	    ecdsa_nid;
	EC_KEY *ecdsa_key;
    /* ed25519 curve */
	unsigned char *ed25519_sk;
	unsigned char *ed25519_pk;
};

/* supported key types */
static const struct {
	const char *name;
	const char *shortname;
	int type;
    int nid;
    int iscert;
} supported_keytypes[] = {
/*    name                                          shortname       type                nid                  cert */
	{ "ssh-ed25519",                                "ED25519",      KEY_ED25519,        0,                      0 },
	{ "ssh-ed25519-cert-v01@openssh.com",           "ED25519-CERT", KEY_ED25519_CERT,   0,                      1 },
    { "ecdsa-sha2-nistp256",                        "ECDSA",        KEY_ECDSA,          NID_X9_62_prime256v1,   0 },
	{ "ecdsa-sha2-nistp256-cert-v01@openssh.com",   "ECDSA-CERT",   KEY_ECDSA_CERT,     NID_X9_62_prime256v1,   1 },
	{ "ecdsa-sha2-nistp384",                        "ECDSA",        KEY_ECDSA,          NID_secp384r1,          0 },
	{ "ecdsa-sha2-nistp384-cert-v01@openssh.com",   "ECDSA-CERT",   KEY_ECDSA_CERT,     NID_secp384r1,          1 },
	{ "ecdsa-sha2-nistp521",                        "ECDSA",        KEY_ECDSA,          NID_secp521r1,          0 },
	{ "ecdsa-sha2-nistp521-cert-v01@openssh.com",   "ECDSA-CERT",   KEY_ECDSA_CERT,     NID_secp521r1,          1 },
};

/* +--------------------------------+ */
/* | newly allocate or cleanly free | */
/* +--------------------------------+ */

/* allocate new*/
struct opensshkey *newopensshkey (int type)
{
    struct opensshkey *newkey;

    if ((newkey = zalloc(sizeof *newkey)) == NULL)
        return NULL;
    
    newkey->type = type;
    newkey->ecdsa_key = NULL;
    newkey->ecdsa_nid = -1;
    newkey->ed25519_sk = NULL;
    newkey->ed25519_pk = NULL;

    return newkey;
}

/* free with explizit zeroing */
void freeopensshkey (struct opensshkey *key)
{
    if (key == NULL)
        return;

    switch (key->type) {

        case KEY_ED25519:
        case KEY_ED25519_CERT:
            /* memzero() pointers and free */
            wipepointer(key->ed25519_pk, ED25519_PUBLICKEY_SIZE);
            wipepointer(key->ed25519_sk, ED25519_SECRETKEY_SIZE);
            break;

        case KEY_ECDSA:
        case KEY_ECDSA_CERT:
        /* requires openssl or libressl ..
            if (key->ecdsa_key != NULL)
                EC_KEY_free(key->ecdsa_key);
            key->ecdsa_key = NULL;
            break; */
        
        case KEY_UNKNOWN:
        default:
            break;

    }

    /* free struct itself */
    wipepointer(key, sizeof *key);
    return;
}


/* +---------------------+ */
/* | misc key operations | */
/* +---------------------+ */

/* detect key type from given string */
int opensshkey_detect_type (const unsigned char *name)
{
    int numsupported = sizeof supported_keytypes / sizeof *supported_keytypes;
    for (int i = 0; i < numsupported; i++) {
        if (supported_keytypes[i].name == NULL || supported_keytypes[i].shortname == NULL)
            continue;
            /* match long name */
        if (strcmp(name, supported_keytypes[i].name) == 0 ||
            strcasecmp(name, supported_keytypes[i].shortname) == 0)
                return supported_keytypes[i].type;
    }
    return KEY_UNKNOWN;
}

int opensshkey_set_ed25519_keys (struct opensshkey *key, unsigned char *pk, unsigned char *sk)
{
    if (key == NULL)
        return OPENSSH_KEY_NULLPOINTER;

    /* key must be ed25519 key */
    if ( !(key->type == KEY_ED25519 || key->type == KEY_ED25519_CERT) )
        return OPENSSH_KEY_INCOMPATIBLE;

    /* set new public key */
    if (pk != NULL)
        key->ed25519_pk = pk;

    if (sk != NULL)
        key->ed25519_sk = sk;
    
    return OPENSSH_KEY_SUCCESS;
    
}