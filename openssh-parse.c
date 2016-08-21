#include "openssh-parse.h"

/* for debugging */
#include <stdio.h>

/* early exit, e.g. if some condition failed, but do cleanup first! */
#define early_exit(status) do { e = (status); goto cleanup; } while (0)

/* parse key from a openssh-key-v1 formatted filebuffer */
int openssh_key_v1_parse (struct buffer *filebuf)
{
    int e = OPENSSH_KEY_FAILURE;
    struct buffer *encoded = NULL, *decoded = NULL, *privatekeyblob = NULL;
    
    /* allocate temporary buffers for decoding */
    if ((encoded = newbuffer()) == NULL || (decoded = newbuffer()) == NULL)
        early_exit(BUFFER_ALLOCATION_FAILED);

    printf("buffers allocated ..\n");

    /* check the existence of starting mark (aka. preamble) */
    const unsigned char *rawptr = buffer_get_dataptr(filebuf);
                  size_t rawlen = buffer_get_remaining(filebuf);

    printf("checking preamble: %.*s\n", OPENSSH_KEY_V1_MARK_BEGIN_LEN - 1, rawptr);

    /* length greater than MARKs and preamble matches */
    if (rawlen < (OPENSSH_KEY_V1_MARK_BEGIN_LEN + OPENSSH_KEY_V1_MARK_END_LEN) ||
        memcmp(rawptr, OPENSSH_KEY_V1_MARK_BEGIN, OPENSSH_KEY_V1_MARK_BEGIN_LEN) != 0)
            return OPENSSH_KEY_INVALID_FORMAT;

    printf("preamble OK\n");

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

            printf("newline encountered with %d bytes remaining\n", rawlen);
            printf("checking end marker: %.*s\n", OPENSSH_KEY_V1_MARK_END_LEN - 1, rawptr);

            if (rawlen >= OPENSSH_KEY_V1_MARK_END_LEN &&
                memcmp(rawptr, OPENSSH_KEY_V1_MARK_END, OPENSSH_KEY_V1_MARK_END_LEN) == 0) {
                    
                    printf("end marker matched!\n");

                    /* end marker matched, terminate with a nullchar */
                    if ((e = buffer_put_char(encoded, '\0')) != BUFFER_SUCCESS)
                        early_exit(e);
                    break;
                }
        }
    }
    /* we may have reached the end without an end marker */
    if (rawlen == 0)
        early_exit(OPENSSH_KEY_INVALID_FORMAT);

    printf("\ndecode the buffer from base64 ..\n");

    /* base64 decode the buffer */
    if ((e = buffer_put_decoded_base64(decoded, (char *)buffer_get_dataptr(encoded)) ) != BUFFER_SUCCESS)
        early_exit(e);

    /* DEBUG dump the decoded buffer */
    printf("dump decoded buffer:");
    buffer_dump(decoded);

    printf("check MAGIC BYTES: %.*s\n", OPENSSH_KEY_V1_MAGICBYTES_LEN, buffer_get_dataptr(decoded));
    
    /* check magic bytes */
    if (buffer_get_remaining(decoded) < OPENSSH_KEY_V1_MAGICBYTES_LEN ||
        memcmp(buffer_get_dataptr(decoded), OPENSSH_KEY_V1_MAGICBYTES, OPENSSH_KEY_V1_MAGICBYTES_LEN) ||
        buffer_add_offset(decoded, OPENSSH_KEY_V1_MAGICBYTES_LEN) != BUFFER_SUCCESS)
            early_exit(OPENSSH_KEY_INVALID_FORMAT);

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

    buffer_dump(decoded);
    printf( "I've got these values:\n"
            " ciphername:  %s\n"
            " kdf name:    %s\n"
            " no of keys:  %lu\n"
            " private len: %lu\n",
            ciphername, kdfname, nkeys, privatelen);

    /* don't support encryption yet, cipher and kdf need to be 'none' */
    if (strncmp(ciphername, "none", 4) != 0)
        early_exit(OPENSSH_KEY_UNSUPPORTED_CIPHER);
    if (strncmp(kdfname, "none", 4) != 0)
        early_exit(OPENSSH_KEY_UNSUPPORTED_KDF);

    /* need exactly one key */
    if (nkeys != 1)
        early_exit(OPENSSH_KEY_UNSUPPORTED_MULTIPLEKEYS);

    /* privatekey length must correspond to blocksize and remaining buffer */
    if ( privatelen < OPENSSH_KEY_NOCIPHER_BLOCKSIZE       ||
        (privatelen % OPENSSH_KEY_NOCIPHER_BLOCKSIZE) != 0 ||
        buffer_get_remaining(decoded) != privatelen )
            early_exit(OPENSSH_KEY_INVALID_PRIVATE_FORMAT);
    
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
        early_exit(OPENSSH_KEY_INVALID_PRIVATE_FORMAT);

    /* deserialize key and get comment */
    openssh_private_deserialize(privatekeyblob, NULL);

    /* early exit or regular cleanup */
    cleanup:
        freebuffer(encoded);
        freebuffer(decoded);
        freebuffer(privatekeyblob);

    return e;

}

/* deserialize key from buffer */
int openssh_private_deserialize (struct buffer *buf, struct opensshkey **keyptr)
{
    int e = OPENSSH_KEY_FAILURE;
    
    printf("\n ==== DESERIALIZE KEY ====\n");

    unsigned char *typename = NULL;
    if ((e = buffer_read_string(buf, &typename, NULL, '\0')) != BUFFER_SUCCESS)
        early_exit(e);

    printf("typename: %s\n", typename);

    
    cleanup:
        printf("cleanup: nothing to do yet.\n");

}