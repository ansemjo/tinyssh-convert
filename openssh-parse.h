#ifndef _headerguard_openssh_parse_h_
#define _headerguard_openssh_parse_h_

#include "buffer.h"
#include "openssh-key.h"

/****************************************************************************************/

/* openssh-key-v1 file constants */
#define OPENSSH_KEY_V1_MARK_BEGIN       "-----BEGIN OPENSSH PRIVATE KEY-----\n"
#define OPENSSH_KEY_V1_MARK_BEGIN_LEN   ( sizeof(OPENSSH_KEY_V1_MARK_BEGIN) - 1 )

#define OPENSSH_KEY_V1_MARK_END         "-----END OPENSSH PRIVATE KEY-----\n"
#define OPENSSH_KEY_V1_MARK_END_LEN     ( sizeof(OPENSSH_KEY_V1_MARK_END)   - 1 )

#define OPENSSH_KEY_V1_MAGICBYTES       "openssh-key-v1"
#define OPENSSH_KEY_V1_MAGICBYTES_LEN   sizeof(OPENSSH_KEY_V1_MAGICBYTES)

/* sizes for cipher = none */
#define OPENSSH_PARSE_NOCIPHER_BLOCKSIZE  8

/* openssh-key-v1 base64 decoded format:

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
*/

/* error codes */
enum openssh_parse_status {
    OPENSSH_PARSE_SUCCESS =   0,
    OPENSSH_PARSE_FAILURE = -30,

    OPENSSH_PARSE_INVALID_FORMAT,
    OPENSSH_PARSE_INVALID_PRIVATE_FORMAT,
    OPENSSH_PARSE_UNSUPPORTED_CIPHER,
    OPENSSH_PARSE_UNSUPPORTED_KDF,
    OPENSSH_PARSE_UNSUPPORTED_MULTIPLEKEYS,
    OPENSSH_PARSE_UNSUPPORTED_KEY_TYPE,
    OPENSSH_PARSE_ALLOCATION_FAILURE,
    OPENSSH_PARSE_INTERNAL_ERROR,
};

/****************************************************************************************/

/* decode a filebuffer */
int openssh_key_v1_parse (struct buffer *filebuf, struct opensshkey **keyptr);

/* deserialize a private key blob */
int openssh_deserialize_private (struct buffer *buf, struct opensshkey **keyptr);

#endif