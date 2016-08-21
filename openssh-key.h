#ifndef _headerguard_openssh_key_h_
#define _headerguard_openssh_key_h_

#include "buffer.h"

/* openssh-key-v1 format:

    byte[]  AUTH_MAGIC
    string  ciphername
    string  kdfname
    string  kdfoptions
    int     number of keys N
    string  publickey1
    string  publickey2
    ...
    string  publickeyN
    string  encrypted, padded list of private keys

    where:
     - int is a 32 bit long unsigned number = uint32
     - string is a concatenation of a uint32 length and
        string of appropriate length
/*

/* reading openssh-key-v1 constants */
#define OPENSSH_KEY_V1_MARK_BEGIN       "-----BEGIN OPENSSH PRIVATE KEY-----\n"
#define OPENSSH_KEY_V1_MARK_BEGIN_LEN   ( sizeof(OPENSSH_KEY_V1_MARK_BEGIN) - 1 )

#define OPENSSH_KEY_V1_MARK_END         "-----END OPENSSH PRIVATE KEY-----\n"
#define OPENSSH_KEY_V1_MARK_END_LEN     ( sizeof(OPENSSH_KEY_V1_MARK_END)   - 1 )

#define OPENSSH_KEY_V1_MAGICBYTES       "openssh-key-v1"
#define OPENSSH_KEY_V1_MAGICBYTES_LEN   sizeof(OPENSSH_KEY_V1_MAGICBYTES)

/* error codes */
enum openssh_key_status {
    OPENSSH_KEY_SUCCESS =   0,
    OPENSSH_KEY_FAILURE = -30,

    OPENSSH_KEY_INVALID_FORMAT,
};

/* decode buffer */
int openssh_key_v1_parse (struct buffer *filebuf);

#endif