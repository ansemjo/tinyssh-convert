#include "openssh-parse.h"

/* parse key from a openssh-key-v1 formatted filebuffer */
int openssh_key_v1_parse (struct buffer *filebuf, struct opensshkey **keyptr)
{
    int e = OPENSSH_PARSE_FAILURE;
    struct buffer *encoded = NULL, *decoded = NULL, *privatekeyblob = NULL;
    
    /* allocate temporary buffers for decoding */
    if ((encoded = newbuffer()) == NULL || (decoded = newbuffer()) == NULL)
        early_exit(BUFFER_ALLOCATION_FAILED);

    /* check the existence of starting mark (aka. preamble) */
    const unsigned char *rawptr = buffer_get_dataptr(filebuf);
                  size_t rawlen = buffer_get_remaining(filebuf);

    /* length greater than MARKs and preamble matches */
    if (rawlen < (OPENSSH_KEY_V1_MARK_BEGIN_LEN + OPENSSH_KEY_V1_MARK_END_LEN) ||
        memcmp(rawptr, OPENSSH_KEY_V1_MARK_BEGIN, OPENSSH_KEY_V1_MARK_BEGIN_LEN) != 0)
            return OPENSSH_PARSE_INVALID_FORMAT;

    /* increment pointer, decrement rem. length */
    rawptr += OPENSSH_KEY_V1_MARK_BEGIN_LEN;
    rawlen -= OPENSSH_KEY_V1_MARK_BEGIN_LEN;

    /* collect encoded data in buffer, looking for end marker */
    unsigned char lastchar;
    while (rawlen > 0) {
        /* skip whitespace, put into buffer otherwise */
        if (*rawptr != '\n' && *rawptr != '\r')
            if ((e = buffer_put_char(encoded, *rawptr)) != BUFFER_SUCCESS)
                early_exit(e);

        lastchar = *rawptr;
        rawlen--;
        rawptr++;

        /* if newline, the next line might be the end marker */
        if (lastchar == '\n') {

            if (rawlen >= OPENSSH_KEY_V1_MARK_END_LEN &&
                memcmp(rawptr, OPENSSH_KEY_V1_MARK_END, OPENSSH_KEY_V1_MARK_END_LEN) == 0) {
                    
                    /* end marker matched, terminate with a nullchar */
                    if ((e = buffer_put_char(encoded, '\0')) != BUFFER_SUCCESS)
                        early_exit(e);
                    break;
                }
        }
    }
    /* we may have reached the end without an end marker */
    if (rawlen == 0)
        early_exit(OPENSSH_PARSE_INVALID_FORMAT);

    /* base64 decode the buffer */
    if ((e = buffer_put_decoded_base64(decoded, (char *)buffer_get_dataptr(encoded)) ) != BUFFER_SUCCESS)
        early_exit(e);

    /* check magic bytes */
    if (buffer_get_remaining(decoded) < OPENSSH_KEY_V1_MAGICBYTES_LEN ||
        memcmp(buffer_get_dataptr(decoded), OPENSSH_KEY_V1_MAGICBYTES, OPENSSH_KEY_V1_MAGICBYTES_LEN) ||
        buffer_add_offset(decoded, OPENSSH_KEY_V1_MAGICBYTES_LEN) != BUFFER_SUCCESS)
            early_exit(OPENSSH_PARSE_INVALID_FORMAT);

    /* 
     *  begin parsing of the actual key format.
     *  see header for details of format.
     */

    unsigned char *ciphername, *kdfname;
    unsigned long nkeys, privatelen;
    
    if (/*   reading function     buffer   target        len   nullchar   expected status */
        
        /* cipher name */
        (e = buffer_read_string ( decoded, &ciphername,  NULL, '\0' )) != BUFFER_SUCCESS ||
        /* kdf name */
        (e = buffer_read_string ( decoded, &kdfname,     NULL, '\0' )) != BUFFER_SUCCESS ||
        /* skip kdf options */
        (e = buffer_read_string ( decoded, NULL,         NULL, NULL )) != BUFFER_SUCCESS ||
        /* number of keys */
        (e = buffer_read_u32    ( decoded, &nkeys                   )) != BUFFER_SUCCESS ||
        /* skip public key */
        (e = buffer_read_string ( decoded, NULL,         NULL, NULL )) != BUFFER_SUCCESS ||
        /* privatekey blob length */
        (e = buffer_read_u32    ( decoded, &privatelen              )) != BUFFER_SUCCESS
    
    ) early_exit(e);

    /* don't support encryption yet, cipher and kdf need to be 'none' */
    if (strncmp(ciphername, "none", 4) != 0)
        early_exit(OPENSSH_PARSE_UNSUPPORTED_CIPHER);
    if (strncmp(kdfname, "none", 4) != 0)
        early_exit(OPENSSH_PARSE_UNSUPPORTED_KDF);

    /* need exactly one key */
    if (nkeys != 1)
        early_exit(OPENSSH_PARSE_UNSUPPORTED_MULTIPLEKEYS);

    /* privatekey length must correspond to blocksize and remaining buffer */
    if ( privatelen < OPENSSH_PARSE_NOCIPHER_BLOCKSIZE       ||
        (privatelen % OPENSSH_PARSE_NOCIPHER_BLOCKSIZE) != 0 ||
        buffer_get_remaining(decoded) != privatelen )
            early_exit(OPENSSH_PARSE_INVALID_PRIVATE_FORMAT);
    
    /*
     *  usually, decryption would need to be performed at this point.
     *  since I assume most hostkeys will be unencrypted anyway this
     *  is not supported here. openssh's decryption with no cipher
     *  degrades to a simple memcpy into a new buffer.
     */
    if ((e = buffer_new_from_buffer(&privatekeyblob, decoded)) != BUFFER_SUCCESS)
        early_exit(BUFFER_ALLOCATION_FAILED);

    /* verify that both checkint fields hold the same value */
    unsigned long check1, check2;
    if ((e = buffer_read_u32(privatekeyblob, &check1)) != BUFFER_SUCCESS ||
        (e = buffer_read_u32(privatekeyblob, &check2)) != BUFFER_SUCCESS)
            early_exit(e); 
    if (check1 != check2)
        early_exit(OPENSSH_PARSE_INVALID_PRIVATE_FORMAT);

    /* deserialize key */
    struct opensshkey *newkey = NULL;
    if ((e = openssh_deserialize_private(privatekeyblob, &newkey)) != OPENSSH_PARSE_SUCCESS)
        early_exit(e);

    /* get comment for key */
    unsigned char *comment;
    if ((e = buffer_read_string(privatekeyblob, &comment, NULL, '\0')) != OPENSSH_PARSE_SUCCESS)
        early_exit(e);
    printf("Successfully parsed %s key with comment:\n%s\n", opensshkey_get_typename(newkey), comment);

    /* write pointer to parsed key */
    if (keyptr != NULL) {
        *keyptr = newkey;
        newkey = NULL;
    }

    /* early exit or regular cleanup */
    cleanup:
        freebuffer(encoded);
        freebuffer(decoded);
        freebuffer(privatekeyblob);
        freeopensshkey(newkey);

    return e;

}

/* deserialize key from buffer */
int openssh_deserialize_private (struct buffer *buf, struct opensshkey **keyptr)
{
    int e = OPENSSH_PARSE_FAILURE;
    struct opensshkey *newkey;
    
    if (keyptr != NULL)
        *keyptr = NULL;

    /*  'decrypted' privatekey format

        The list of privatekey/comment pairs is padded with the
        bytes 1, 2, 3, ... until the total length is a multiple
        of the cipher block size.

            uint32  checkint
            uint32  checkint
            string  privatekey1
            string  comment1
            string  privatekey2
            string  comment2
            ...
            string  privatekeyN
            string  commentN
            char    1
            char    2
            char    3
            ...
            char    padlen % 255
    */

    /* detect key type */
    int keytype;
    unsigned char *keytypename = NULL; 
    if ((e = buffer_read_string(buf, &keytypename, NULL, '\0')) != BUFFER_SUCCESS)
        early_exit(e);
    if ((keytype = opensshkey_detect_type (keytypename)) == KEY_UNKNOWN)
        early_exit(OPENSSH_PARSE_UNSUPPORTED_KEY_TYPE);
    
    /* temporary key properties */
    unsigned char *ed25519_pk = NULL, *ed25519_sk = NULL;
    size_t pk_len = 0, sk_len = 0;

    /* decide on action */
    switch (keytype) {

        /* ed25519 keys */
        case KEY_ED25519:
        case KEY_ED25519_CERT:

            /* allocate new key */
            if ((newkey = newopensshkey(keytype)) == NULL)
                early_exit(OPENSSH_PARSE_ALLOCATION_FAILURE);

            /* get public and private key from buffer */
            if ((e = buffer_read_string(buf, &ed25519_pk, &pk_len, NULL)) != BUFFER_SUCCESS ||
                (e = buffer_read_string(buf, &ed25519_sk, &sk_len, NULL)) != BUFFER_SUCCESS)
                    early_exit(e);

            /* check read key lengths */
            if (pk_len != ED25519_PUBLICKEY_SIZE || sk_len != ED25519_SECRETKEY_SIZE)
                early_exit(OPENSSH_PARSE_INVALID_FORMAT);
            
            /* write correct pointers */
            opensshkey_set_ed25519_keys(newkey, ed25519_pk, ed25519_sk);
            ed25519_pk = ed25519_sk = NULL;
            
            break;

        /* ecdsa keys currently not supported */
        case KEY_ECDSA:
        case KEY_ECDSA_CERT:

        /* rsa, dsa, or otherwise unknown type */
        case KEY_UNKNOWN:
        default:
            early_exit(OPENSSH_PARSE_INTERNAL_ERROR);
    }

    /* write pointer to deserialized key */
    if (keyptr != NULL) {
        *keyptr = newkey;
        newkey = NULL;
    }

    /* success */
    e = OPENSSH_PARSE_SUCCESS;

    /* housekeeping .. */
    cleanup:
        free(keytypename);
        freeopensshkey(newkey);
        wipepointer(ed25519_pk, pk_len);
        wipepointer(ed25519_sk, sk_len);

    return e;
}