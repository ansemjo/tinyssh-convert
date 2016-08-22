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
/*    name                                          shortname       type                ecdsa_nid             cert */
	{ "ssh-ed25519",                                "ED25519",      KEY_ED25519,        -1,                     0 },
	{ "ssh-ed25519-cert-v01@openssh.com",           "ED25519-CERT", KEY_ED25519_CERT,   -1,                     1 },
    { "ecdsa-sha2-nistp256",                        "ECDSA",        KEY_ECDSA,          NID_X9_62_prime256v1,   0 },
	{ "ecdsa-sha2-nistp256-cert-v01@openssh.com",   "ECDSA-CERT",   KEY_ECDSA_CERT,     NID_X9_62_prime256v1,   1 },
	{ "ecdsa-sha2-nistp384",                        "ECDSA",        KEY_ECDSA,          NID_secp384r1,          0 },
	{ "ecdsa-sha2-nistp384-cert-v01@openssh.com",   "ECDSA-CERT",   KEY_ECDSA_CERT,     NID_secp384r1,          1 },
	{ "ecdsa-sha2-nistp521",                        "ECDSA",        KEY_ECDSA,          NID_secp521r1,          0 },
	{ "ecdsa-sha2-nistp521-cert-v01@openssh.com",   "ECDSA-CERT",   KEY_ECDSA_CERT,     NID_secp521r1,          1 },
    { "unknown",                                    "UNKNOWN",      KEY_UNKNOWN,        -1,                     0 },
};
#define n_supported_keytypes sizeof supported_keytypes / sizeof *supported_keytypes


/* +--------------------------------+ */
/* | newly allocate or cleanly free | */
/* +--------------------------------+ */

/* allocate new*/
struct opensshkey *newopensshkey (int type)
{
    struct opensshkey *newkey;

    if (type > KEY_UNKNOWN)
        return NULL;

    if ((newkey = zalloc(sizeof *newkey)) == NULL)
        return NULL;
    
    newkey->type = type;
    newkey->ecdsa_key = NULL;
    newkey->ecdsa_nid = -1;
    newkey->ed25519_sk = NULL;
    newkey->ed25519_pk = NULL;

    return newkey;
}

/* free with explicit zeroing */
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


/* +--------------------+ */
/* | keytype operations | */
/* +--------------------+ */

/* detect key type from given string */
int opensshkey_detect_type (const unsigned char *name)
{
    for (int i = 0; i < n_supported_keytypes; i++) {
        /* the supported keytype has invalid names */
        if (supported_keytypes[i].name == NULL || supported_keytypes[i].shortname == NULL)
            continue;
        /* match long name */
        if (strcmp(name, supported_keytypes[i].name) == 0 ||
        /* or match short name */
            strcasecmp(name, supported_keytypes[i].shortname) == 0)
                return supported_keytypes[i].type;
    }
    return KEY_UNKNOWN;
}

int opensshkey_get_type (const struct opensshkey *key)
{
    if (key == NULL)
        return KEY_UNSPECIFIED;
    
    return key->type;
}

const unsigned char *opensshkey_get_typename (const struct opensshkey *key)
{
    if (key == NULL)
        return NULL;

    for (int i = 0; i < n_supported_keytypes; i++) {
        /* the supported keytype has invalid names */
        if (supported_keytypes[i].name == NULL || supported_keytypes[i].shortname == NULL)
            continue;
        /* match keytype */
        if (key->type == supported_keytypes[i].type &&
        /* and match nid */
            key->ecdsa_nid == supported_keytypes[i].nid)
                
                return supported_keytypes[i].name;
    }
    return NULL;
}

/* +----------------------------+ */
/* | operations on key material | */
/* +----------------------------+ */

/* set pk and sk on ed25519 key */
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

    /* set new privatekey */
    if (sk != NULL)
        key->ed25519_sk = sk;
    
    return OPENSSH_KEY_SUCCESS;
}

/* return public and private part of elliptic curve keys */
int opensshkey_save_to_tinyssh (const struct opensshkey *key, const unsigned char *dir, size_t dir_len)
{
    if (key == NULL)
        return OPENSSH_KEY_NULLPOINTER;

    int e = OPENSSH_KEY_FAILURE;

    /* allocate a new buffer for key material */
    struct buffer *keybuf = NULL;
    if ((keybuf = newbuffer()) == NULL)
        return BUFFER_ALLOCATION_FAILED;

    /* strings to construct the filenames in */
    unsigned char pubkey_file[dir_len + 64], seckey_file[dir_len + 64];
    strncat(pubkey_file, dir, dir_len);
    strncat(seckey_file, dir, dir_len);

    /* decide by key type */
    switch (key->type) {

        case KEY_ED25519:
        case KEY_ED25519_CERT:

            /* we need a private key but can use
               the embedded public key there */
            if (key->ed25519_sk == NULL)
                early_exit(OPENSSH_KEY_NULLPOINTER);

            /* construct the filenames */
            strncat(pubkey_file, ED25519_PUBLICKEY_NAME, sizeof ED25519_PUBLICKEY_NAME);
            strncat(seckey_file, ED25519_SECRETKEY_NAME, sizeof ED25519_SECRETKEY_NAME);
            
            /* put secretkey into buffer */
            if ((e = buffer_put(keybuf, key->ed25519_sk, ED25519_SECRETKEY_SIZE)) != BUFFER_SUCCESS)
                early_exit(e);

        
        case KEY_ECDSA:
        case KEY_ECDSA_CERT:
            /* not supported yet, requires ssl library */
        
        case KEY_UNKNOWN:
        case KEY_UNSPECIFIED:
        default:
            return OPENSSH_KEY_UNKNOWN_KEYTYPE;
    }

    /* housekeeping .. */
    cleanup:
        freebuffer(keybuf);
    
    return e;
}

/* +-----------+ */
/* | debugging | */
/* +-----------+ */

void opensshkey_dump (const struct opensshkey *key)
{
    
    if (key == NULL) {
        printf("Passed key pointer is NULL!\n");
        return;
    }

    printf("Dumping opensshkey ...\n");

    switch (key->type) {
        case KEY_ECDSA:
        case KEY_ECDSA_CERT:
            printf("Keytype is: ECDSA (unsupported)\n");
            break;
        case KEY_ED25519:
        case KEY_ED25519_CERT:
            printf("Keytype is: ed25519\n");
            debugbuf("ed25519 public key", key->ed25519_pk, ED25519_PUBLICKEY_SIZE);
            debugbuf("ed25519 secret key", key->ed25519_sk, ED25519_SECRETKEY_SIZE);
            break;
        case KEY_UNKNOWN:
        default:
            printf("Keytype is UNKNOWN!\n");
            break;
    }

}