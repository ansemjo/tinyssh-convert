/*
 * This file is governed by Licenses which are listed in
 * the LICENSE file, which shall be included in all copies
 * and redistributions of this project.
 *
 * Some concepts and original work is derived from these files:
 *  - $OpenBSD: sshkey.c,v 1.36 2016/08/03 05:41:57 djm Exp $
 * 
 * Copyright (c) 2000, 2001 Markus Friedl.  All rights reserved.
 * Copyright (c) 2008 Alexander von Gernler.  All rights reserved.
 * Copyright (c) 2010,2011 Damien Miller.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "openssh-parse.h"

/* parse key from a openssh-key-v1 formatted filebuffer */
int openssh_key_v1_parse (struct buffer *filebuf, struct opensshkey **keyptr)
{
    int e = FAILURE;
    struct buffer *encoded = NULL, *decoded = NULL, *privatekeyblob = NULL;
    
    /* allocate temporary buffers for decoding */
    if ((encoded = newbuffer()) == NULL || (decoded = newbuffer()) == NULL)
        cleanreturn(BUFFER_ALLOCATION_FAILED);

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
            if ((e = buffer_put_char(encoded, *rawptr)) != SUCCESS)
                cleanreturn(e);

        lastchar = *rawptr;
        rawlen--;
        rawptr++;

        /* if newline, the next line might be the end marker */
        if (lastchar == '\n') {

            if (rawlen >= OPENSSH_KEY_V1_MARK_END_LEN &&
                memcmp(rawptr, OPENSSH_KEY_V1_MARK_END, OPENSSH_KEY_V1_MARK_END_LEN) == 0) {
                    
                    /* end marker matched, terminate with a nullchar */
                    if ((e = buffer_put_char(encoded, '\0')) != SUCCESS)
                        cleanreturn(e);
                    break;
                }
        }
    }
    /* we may have reached the end without an end marker */
    if (rawlen == 0)
        cleanreturn(OPENSSH_PARSE_INVALID_FORMAT);

    /* base64 decode the buffer */
    if ((e = buffer_put_decoded_base64(decoded, (char *)buffer_get_dataptr(encoded)) ) != SUCCESS)
        cleanreturn(e);

    /* check magic bytes */
    if (buffer_get_remaining(decoded) < OPENSSH_KEY_V1_MAGICBYTES_LEN ||
        memcmp(buffer_get_dataptr(decoded), OPENSSH_KEY_V1_MAGICBYTES, OPENSSH_KEY_V1_MAGICBYTES_LEN) ||
        buffer_add_offset(decoded, OPENSSH_KEY_V1_MAGICBYTES_LEN) != SUCCESS)
            cleanreturn(OPENSSH_PARSE_INVALID_FORMAT);

    /* 
     *  begin parsing of the actual key format.
     *  see header for details of format.
     */

    unsigned char *ciphername, *kdfname;
    unsigned long nkeys, privatelen;
    
    if (/*   reading function     buffer   target        len   nullchar   expected status */
        
        /* cipher name */
        (e = buffer_read_string ( decoded, &ciphername,  NULL, '\0' )) != SUCCESS ||
        /* kdf name */
        (e = buffer_read_string ( decoded, &kdfname,     NULL, '\0' )) != SUCCESS ||
        /* skip kdf options */
        (e = buffer_read_string ( decoded, NULL,         NULL, NULL )) != SUCCESS ||
        /* number of keys */
        (e = buffer_read_u32    ( decoded, &nkeys                   )) != SUCCESS ||
        /* skip public key */
        (e = buffer_read_string ( decoded, NULL,         NULL, NULL )) != SUCCESS ||
        /* privatekey blob length */
        (e = buffer_read_u32    ( decoded, &privatelen              )) != SUCCESS
    
    ) cleanreturn(e);

    /* don't support encryption yet, cipher and kdf need to be 'none' */
    if (strncmp(ciphername, "none", 4) != 0)
        cleanreturn(OPENSSH_PARSE_UNSUPPORTED_CIPHER);
    if (strncmp(kdfname, "none", 4) != 0)
        cleanreturn(OPENSSH_PARSE_UNSUPPORTED_KDF);

    /* need exactly one key */
    if (nkeys != 1)
        cleanreturn(OPENSSH_PARSE_UNSUPPORTED_MULTIPLEKEYS);

    /* privatekey length must correspond to blocksize and remaining buffer */
    if ( privatelen < OPENSSH_PARSE_NOCIPHER_BLOCKSIZE       ||
        (privatelen % OPENSSH_PARSE_NOCIPHER_BLOCKSIZE) != 0 ||
        buffer_get_remaining(decoded) != privatelen )
            cleanreturn(OPENSSH_PARSE_INVALID_PRIVATE_FORMAT);
    
    /*
     *  usually, decryption would need to be performed at this point.
     *  since I assume most hostkeys will be unencrypted anyway this
     *  is not supported here. openssh's decryption with no cipher
     *  degrades to a simple memcpy into a new buffer.
     */
    if ((e = buffer_new_from_buffer(&privatekeyblob, decoded)) != SUCCESS)
        cleanreturn(BUFFER_ALLOCATION_FAILED);

    /* verify that both checkint fields hold the same value */
    unsigned long check1, check2;
    if ((e = buffer_read_u32(privatekeyblob, &check1)) != SUCCESS ||
        (e = buffer_read_u32(privatekeyblob, &check2)) != SUCCESS)
            cleanreturn(e); 
    if (check1 != check2)
        cleanreturn(OPENSSH_PARSE_INVALID_PRIVATE_FORMAT);

    /* deserialize key */
    struct opensshkey *newkey = NULL;
    if ((e = openssh_deserialize_private(privatekeyblob, &newkey)) != SUCCESS)
        cleanreturn(e);

    /* get comment for key */
    unsigned char *comment;
    if ((e = buffer_read_string(privatekeyblob, &comment, NULL, '\0')) != SUCCESS)
        cleanreturn(e);
    printf("Successfully parsed %s key with comment: %s\n", opensshkey_get_typename(newkey), comment);

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
    int e = FAILURE;
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
    if ((e = buffer_read_string(buf, &keytypename, NULL, '\0')) != SUCCESS)
        cleanreturn(e);
    if ((keytype = opensshkey_detect_type (keytypename)) == KEY_UNKNOWN)
        cleanreturn(OPENSSH_PARSE_UNSUPPORTED_KEY_TYPE);
    
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
                cleanreturn(OPENSSH_KEY_ALLOCATION_FAILURE);

            /* get public and private key from buffer */
            if ((e = buffer_read_string(buf, &ed25519_pk, &pk_len, NULL)) != SUCCESS ||
                (e = buffer_read_string(buf, &ed25519_sk, &sk_len, NULL)) != SUCCESS)
                    cleanreturn(e);

            /* check read key lengths */
            if (pk_len != ED25519_PUBLICKEY_SIZE || sk_len != ED25519_SECRETKEY_SIZE)
                cleanreturn(OPENSSH_PARSE_INVALID_FORMAT);
            
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
            cleanreturn(OPENSSH_PARSE_INTERNAL_ERROR);
    }

    /* write pointer to deserialized key */
    if (keyptr != NULL) {
        *keyptr = newkey;
        newkey = NULL;
    }

    /* success */
    e = SUCCESS;

    /* housekeeping .. */
    cleanup:
        free(keytypename);
        freeopensshkey(newkey);
        nullpointer(ed25519_pk, pk_len);
        nullpointer(ed25519_sk, sk_len);

    return e;
}