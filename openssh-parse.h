/*
 * This file is governed by Licenses which are listed in
 * the LICENSE file, which shall be included in all copies
 * and redistributions of this project.
 *
 * For additional notices see the file openssh-parse.c
 */

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

/* statuscodes are in statuscodes.h */

/****************************************************************************************/

/* decode a filebuffer */
int openssh_key_v1_parse (struct buffer *filebuf, struct opensshkey **keyptr);

/* deserialize a private key blob */
int openssh_deserialize_private (struct buffer *buf, struct opensshkey **keyptr);

#endif