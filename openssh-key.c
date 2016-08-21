#include "openssh-key.h"

/* for debugging */
#include <stdio.h>

/* early exit, e.g. if some condition failed, but do cleanup first! */
#define early_exit(status) do { e = (status); goto cleanup; } while (0)

/* parse key from a openssh-key-v1 formatted filebuffer */
int openssh_key_v1_parse (struct buffer *filebuf)
{
    int e = OPENSSH_KEY_FAILURE;

    /* allocate temporary buffers for decoding */
    struct buffer *encoded = NULL, *decoded = NULL;
    if ((encoded = newbuffer()) == NULL || (decoded = newbuffer()) == NULL)
        early_exit(BUFFER_ALLOCATION_FAILED);

    printf("buffers allocated ..\n");

    /* check the existence of starting mark (aka. preamble) */
    const unsigned char *rawptr;
    size_t rawlen;
    rawptr = buffer_get_dataptr(filebuf);
    rawlen = buffer_get_remaining(filebuf);

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
    if ((e = buffer_decode_from_base64(decoded, (char *)buffer_get_dataptr(encoded))) != BUFFER_SUCCESS)
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
    unsigned char *debugstring;
    unsigned long debugint;
    
    /* cipher name */
    if ((e = buffer_read_string(decoded, &debugstring, NULL, '\0')) != BUFFER_SUCCESS)
        early_exit(e);
    printf("read CIPHER NAME:  %s\n", debugstring);

    if ((e = buffer_read_string(decoded, &debugstring, NULL, '\0')) != BUFFER_SUCCESS)
        early_exit(e);
    printf("read KDF NAME:     %s\n", debugstring);

    if ((e = buffer_read_string(decoded, &debugstring, NULL, NULL)) != BUFFER_SUCCESS)
        early_exit(e);
    printf("read KDF OPTIONS:  %s\n", debugstring);

    if ((e = buffer_read_u32(decoded, &debugint)) != BUFFER_SUCCESS)
        early_exit(e);
    printf("read N. OF KEYS:   %d\n", debugint);

    unsigned char *pubkey;
    size_t pubkeylen;
    if ((e = buffer_read_string(decoded, &pubkey, &pubkeylen, NULL)) != BUFFER_SUCCESS)
        early_exit(e);
    debugbuf("PUBLIC KEY", pubkey, pubkeylen);

    if ((e = buffer_read_u32(decoded, &debugint)) != BUFFER_SUCCESS)
        early_exit(e);
    printf("length of encrypted blob: %d bytes\n", debugint);

    buffer_dump(decoded);

    


    /* early exit or regular cleanup */
    cleanup:
        freebuffer(encoded);
        freebuffer(decoded);

    return e;

}