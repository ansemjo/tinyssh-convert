/*
    THIS FILE IS NOT REALLY A HEADERFILE!
    THIS IS A TEMPORARY COPY OF VARIOUS FUNCTIONS
    FROM THE OPENSSH PROJECT TO WORK ON
*/
#ifdef _DO_NOT_INCLUDE_5WDFkGlJ6bUsH4FnmztwwIiKyfftBm8I_


/*
    openssh-key-v1 format and parsing ..
    Duh'. There is public documentation available on the format:
    http://cvsweb.openbsd.org/cgi-bin/cvsweb/src/usr.bin/ssh/PROTOCOL.key?annotate=HEAD
*/


static int sshkey_parse_private2 (
    struct sshbuf *blob,
    int type,
    const char *passphrase,
    struct sshkey **keyp,
    char **commentp
)
{
	char *comment = NULL, *ciphername = NULL, *kdfname = NULL;
	const struct sshcipher *cipher = NULL;
	const u_char *cp;
	int r = SSH_ERR_INTERNAL_ERROR;
	size_t encoded_len;
	size_t i, keylen = 0, ivlen = 0, authlen = 0, slen = 0;
	struct sshbuf *encoded = NULL, *decoded = NULL;
	struct sshbuf *kdf = NULL, *decrypted = NULL;
	struct sshcipher_ctx *ciphercontext = NULL;
	struct sshkey *k = NULL;
	u_char *key = NULL, *salt = NULL, *dp, pad, last;
	u_int blocksize, rounds, nkeys, encrypted_len, check1, check2;

	if (keyp != NULL) *keyp = NULL;
	if (commentp != NULL) *commentp = NULL;

    /* allocate mem */
	if ((encoded = sshbuf_new()) == NULL ||
	    (decoded = sshbuf_new()) == NULL ||
	    (decrypted = sshbuf_new()) == NULL) {
		r = SSH_ERR_ALLOC_FAIL;
		goto out;
	}

/* openssh private key file format */
#define MARK_BEGIN		"-----BEGIN OPENSSH PRIVATE KEY-----\n"
#define MARK_END		"-----END OPENSSH PRIVATE KEY-----\n"
#define MARK_BEGIN_LEN		(sizeof(MARK_BEGIN) - 1)
#define MARK_END_LEN		(sizeof(MARK_END) - 1)
#define KDFNAME			"bcrypt"
#define AUTH_MAGIC		"openssh-key-v1"
#define SALT_LEN		16
#define DEFAULT_CIPHERNAME	"aes256-cbc"
#define	DEFAULT_ROUNDS		16


    /* check the first line (MARK_BEGIN) and advance cp pointer */
	/* check preamble */
	cp = sshbuf_ptr(blob);
	encoded_len = sshbuf_len(blob);
	if (encoded_len < (MARK_BEGIN_LEN + MARK_END_LEN) ||
	    memcmp(cp, MARK_BEGIN, MARK_BEGIN_LEN) != 0) {
		r = SSH_ERR_INVALID_FORMAT;
		goto out;
	}
	cp += MARK_BEGIN_LEN;
	encoded_len -= MARK_BEGIN_LEN;


    /* 'collect' all encoded lines in 'encoded' */
	/* Look for end marker, removing whitespace as we go */
	while (encoded_len > 0) {
		if (*cp != '\n' && *cp != '\r') {
			if ((r = sshbuf_put_u8(encoded, *cp)) != 0) /* pick single chars at a time */
				goto out;
		}

		last = *cp;
		encoded_len--;
		cp++;
		if (last == '\n') { /* if the last one was a newline .. */
            /* check if the next bytes fit the END MARKER */
			if (encoded_len >= MARK_END_LEN && memcmp(cp, MARK_END, MARK_END_LEN) == 0) {
				/* \0 terminate */
				if ((r = sshbuf_put_u8(encoded, 0)) != 0)
					goto out;
				break;
			}
		}
	}
    /* something went horribly wrong here */
	if (encoded_len == 0) {
		r = SSH_ERR_INVALID_FORMAT; goto out; }


	/* decode base64 */
	if ((r = sshbuf_b64tod(decoded, (char *)sshbuf_ptr(encoded))) != 0)
		goto out;

    /* AUTH_MAGIC = openssh-key-v1 */
	/* check magic */
	if (sshbuf_len(decoded) < sizeof(AUTH_MAGIC) ||
	    memcmp(sshbuf_ptr(decoded), AUTH_MAGIC, sizeof(AUTH_MAGIC))) {
		r = SSH_ERR_INVALID_FORMAT;
		goto out;
	}


/* parse public portion of key, 'decoded' is sshbuf after base64 decode */
    /* many functions from sshbuf-getput-basic.c are used here */

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
            \0-terminated string of appropriate length
    /*

        /* consume moves offset forward by len, i.e. skip pointer over MAGIC */
	if ((r = sshbuf_consume(decoded, sizeof(AUTH_MAGIC))) != 0 ||
        
        /* detect and read the next size+string blob as the used CIPHER name */
	    (r = sshbuf_get_cstring(decoded, &ciphername, NULL)) != 0 ||

        /* detect and read the next size+string blob as the KDF function name */
	    (r = sshbuf_get_cstring(decoded, &kdfname, NULL)) != 0 ||

        /* new buffer from the next string, 
           which is part of the key derivation but
           is only used if kdfname == bcrypt */
	    (r = sshbuf_froms(decoded, &kdf)) != 0 ||

        /* get next 4 bytes as uint32 number, should be 0000..0001 */
	    (r = sshbuf_get_u32(decoded, &nkeys)) != 0 ||

        /* simply skip the next string, pubkeys are here */
	    (r = sshbuf_skip_string(decoded)) != 0 ||

        /* get length of encrypted blob as uint32 */
	    (r = sshbuf_get_u32(decoded, &encrypted_len)) != 0)

	goto out;


/* checks before starting on privatekey .. */

    /* get cipher by name */
	if ((cipher = cipher_by_name(ciphername)) == NULL) {
		r = SSH_ERR_KEY_UNKNOWN_CIPHER; goto out; }

    /* no passphrase, but cipher is not none ? */
	if ((passphrase == NULL || strlen(passphrase) == 0) &&
	    strcmp(ciphername, "none") != 0) {
		r = SSH_ERR_KEY_WRONG_PASSPHRASE; goto out; }

    /* KDF is neither none nor bcrypt ? */
	if (strcmp(kdfname, "none") != 0 && strcmp(kdfname, "bcrypt") != 0) {
		r = SSH_ERR_KEY_UNKNOWN_CIPHER; goto out; }

    /* no KDF given, but cipher not none? */
	if (!strcmp(kdfname, "none") && strcmp(ciphername, "none") != 0) {
		r = SSH_ERR_INVALID_FORMAT;	goto out; }

    /* more than one key? wtf?! */
	if (nkeys != 1) {
		r = SSH_ERR_INVALID_FORMAT;	goto out; }

	/* check size of encrypted key blob */
	blocksize = cipher_blocksize(cipher);
    /*  not smaller than blocksize and divisible by blocksize ! */
	if (encrypted_len < blocksize || (encrypted_len % blocksize) != 0) {
		r = SSH_ERR_INVALID_FORMAT; goto out;
	}

	/* setup decryption parameters */
	keylen  = cipher_keylen(cipher);
	ivlen   = cipher_ivlen(cipher);
	authlen = cipher_authlen(cipher);

    /* allocate memory */
	if ((key = calloc(1, keylen + ivlen)) == NULL) {
		r = SSH_ERR_ALLOC_FAIL; goto out; }
    
    /* if bcrypt is used .. perform key derivation */
	if (strcmp(kdfname, "bcrypt") == 0) {
        /* parse salt and kdf rounds from kdf buffer */
        /*  string salt
            uint32 rounds   */
		if ((r = sshbuf_get_string(kdf, &salt, &slen)) != 0 ||
		    (r = sshbuf_get_u32(kdf, &rounds)) != 0)
			goto out;
        /* perform key derivation and store in 'key' */
		if (bcrypt_pbkdf(passphrase, strlen(passphrase), salt, slen,
		    key, keylen + ivlen, rounds) < 0) {
			r = SSH_ERR_INVALID_FORMAT;
			goto out;
		}
	}

	/* check that an appropriate amount of auth data is present */
	if (sshbuf_len(decoded) < encrypted_len + authlen) {
		r = SSH_ERR_INVALID_FORMAT;	goto out; }


	/* decrypt private portion of key */
    /*
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
    /* allocate mem */
	if ((r = sshbuf_reserve(decrypted, encrypted_len, &dp)) != 0 ||
	    (r = cipher_init(&ciphercontext, cipher, key, keylen,
	    key + keylen, ivlen, 0)) != 0) goto out;

    /* perform decryption of 'decoded' into 'dp' */
	if ((r = cipher_crypt(ciphercontext, 0, dp, sshbuf_ptr(decoded),
	    encrypted_len, 0, authlen)) != 0) {
		/* an integrity error here indicates an incorrect passphrase */
		if (r == SSH_ERR_MAC_INVALID) r = SSH_ERR_KEY_WRONG_PASSPHRASE;
		goto out;
	}

    /* advance pointer over encrypted data */
	if ((r = sshbuf_consume(decoded, encrypted_len + authlen)) != 0) goto out;
	/* there should be no trailing data now */
	if (sshbuf_len(decoded) != 0) {
		r = SSH_ERR_INVALID_FORMAT; goto out; }


 /* Before the key is encrypted, a random integer is assigned
    to both checkint fields so successful decryption can be
    quickly checked by verifying that both checkint fields
    hold the same value. */
	/* check check bytes */
	if ((r = sshbuf_get_u32(decrypted, &check1)) != 0 ||
	    (r = sshbuf_get_u32(decrypted, &check2)) != 0) goto out;
	if (check1 != check2) {
		r = SSH_ERR_KEY_WRONG_PASSPHRASE; goto out; }


	/* Load the private key and comment */
          /* function at end of file */
	if ((r = sshkey_private_deserialize(decrypted, &k)) != 0 ||
	    (r = sshbuf_get_cstring(decrypted, &comment, NULL)) != 0)
        /* no more than one .. there's not supposed to be any more */
		goto out;


	/* Check deterministic padding */
	i = 0;
    /* while not at end of buffer */
	while (sshbuf_len(decrypted)) {
        /* load next pad as unsigned 8 bit int */
		if ((r = sshbuf_get_u8(decrypted, &pad)) != 0)
            goto out;
        /* that pad should be counting upwards until blocksize is reached */
		if (pad != (++i & 0xff)) {
			r = SSH_ERR_INVALID_FORMAT;	goto out;
		}
	}

	/* XXX decode pubkey and check against private */
    /* well .. no. */

	/* success */
	r = 0;
    /* point given keypointer to key */
	if (keyp != NULL) {
		*keyp = k;
		k = NULL;
	}
    /* point given commentpointer to comment */
	if (commentp != NULL) {
		*commentp = comment;
		comment = NULL;
	}

/* housekeeping .. */
/* remind me to use explicitly zeroing free's .. */
 out:
	pad = 0;
	cipher_free(ciphercontext);
	free(ciphername);
	free(kdfname);
	free(comment);
	if (salt != NULL) {
		explicit_bzero(salt, slen);
		free(salt);
	}
	if (key != NULL) {
		explicit_bzero(key, keylen + ivlen);
		free(key);
	}
	sshbuf_free(encoded);
	sshbuf_free(decoded);
	sshbuf_free(kdf);
	sshbuf_free(decrypted);
	sshkey_free(k);
	return r;
}


/***************************************************/

/*
    function used to deserialize a decrypted string to
    a sshkey struct and thereby advancing the offset on the buf

    .. gets a lot shorter if you cut out the openssl stuff. :D
*/

int sshkey_private_deserialize (struct sshbuf *buf, struct sshkey **keypointer)
{
	char *typename = NULL;
	struct sshkey *tmpkey = NULL;
	size_t pubk_len = 0, seck_len = 0;
	int type, r = SSH_ERR_INTERNAL_ERROR;
	u_char *ed25519_pk = NULL, *ed25519_sk = NULL;

    /* zero target contents */
	if (keypointer != NULL) *keypointer = NULL;

    /* get typename from buffer and detect */
	if ((r = sshbuf_get_cstring(buf, &typename, NULL)) != 0) goto out;
	type = sshkey_type_from_name(typename);

    /* many more cases with openssl! */
	switch (type) {
        
        case KEY_ED25519:
        /*  string  ed25519_pk
            string  ed25519_sk  */

            /* allocate mem */
            if ((tmpkey = sshkey_new_private(type)) == NULL) {
                r = SSH_ERR_ALLOC_FAIL; goto out; }

            /* get public key and secret key consecutively from buf */
            if ((r = sshbuf_get_string(buf, &ed25519_pk, &pubk_len)) != 0 ||
                (r = sshbuf_get_string(buf, &ed25519_sk, &seck_len)) != 0) goto out;

            /* check their length */
            if (pubk_len != ED25519_PK_SZ || seck_len != ED25519_SK_SZ) {
                r = SSH_ERR_INVALID_FORMAT; goto out; }
            
            /* assign to temporary key and zero */
            tmpkey->ed25519_pk = ed25519_pk;
            tmpkey->ed25519_sk = ed25519_sk;
            ed25519_pk = ed25519_sk = NULL;
            break;

        case KEY_ED25519_CERT:

                /* parse next string as key to include the certblob? */
            if ((r = sshkey_froms(buf, &tmpkey)) != 0 ||
                /* does nothing for ed25519
                (r = sshkey_add_private(tmpkey)) != 0 || */

                /* get pub and sec key from buf as above */
                (r = sshbuf_get_string(buf, &ed25519_pk, &pubk_len)) != 0 ||
                (r = sshbuf_get_string(buf, &ed25519_sk, &seck_len)) != 0) goto out;

            /* check their length */
            if (pubk_len != ED25519_PK_SZ || seck_len != ED25519_SK_SZ) {
                r = SSH_ERR_INVALID_FORMAT; goto out; }

            /* assign to temporary key and zero */
            tmpkey->ed25519_pk = ed25519_pk;
            tmpkey->ed25519_sk = ed25519_sk;
            ed25519_pk = ed25519_sk = NULL;
            break;

        default:
            /* unknown type .. */
            r = SSH_ERR_KEY_TYPE_UNKNOWN;
            goto out;
	}

	/* success */
	r = 0;

    /* copy to keypointer and zero temp copy */
	if (keypointer != NULL) {
		*keypointer = tmpkey;
		tmpkey = NULL;
	}

/* housekeeping ... */
 out:
	free(typename);
	sshkey_free(tmpkey);

    /* explizitly zero the keyparts */
	if (ed25519_pk != NULL) {
		explicit_bzero(ed25519_pk, pubk_len);
		free(ed25519_pk);
	}
	if (ed25519_sk != NULL) {
		explicit_bzero(ed25519_sk, seck_len);
		free(ed25519_sk);
	}

	return r;
}


/***************************************************/

/*
    sshbuf functions to get and parse strings ..
*/


/* get next uint32 number */
int sshbuf_get_u32(struct sshbuf *buf, u_int32_t *valp)
{
	const u_char *bufpointer = sshbuf_ptr(buf);
	int r;

    /* advance 4 bytes */
	if ((r = sshbuf_consume(buf, 4)) < 0) return r;

    /* put those 4 bytes as 32 bit uint number */
	if (valp != NULL) *valp = PEEK_U32(bufpointer);
	return 0;
}

/* get next uint8 number */
int sshbuf_get_u8(struct sshbuf *buf, u_char *valp)
{
	const u_char *p = sshbuf_ptr(buf);
	int r;

    /* advance 1 byte */
	if ((r = sshbuf_consume(buf, 1)) < 0) return r;

    /* put that byte as 8 bit uint */
	if (valp != NULL) *valp = (u_int8_t)*p;
	return 0;
}

/* get string by allocating new at *valp and no checking for \0 term */
int sshbuf_get_string (struct sshbuf *buf, u_char **valp, size_t *lenp)
{
	const u_char *val;
	size_t len;
	int r;

    /* reset */
	if (valp != NULL) *valp = NULL;
	if (lenp != NULL) *lenp = 0;

    /* get address + length of string and advance offset */
	if ((r = sshbuf_get_string_direct(buf, &val, &len)) < 0) return r;
	
    if (valp != NULL) {
        /* allocate mem */
		if ((*valp = malloc(len + 1)) == NULL) {
			SSHBUF_DBG(("SSH_ERR_ALLOC_FAIL"));
			return SSH_ERR_ALLOC_FAIL;
		}
        /* copy string */
		if (len != 0) memcpy(*valp, val, len);
		(*valp)[len] = '\0';
	}

	if (lenp != NULL) *lenp = len;
	return 0;
}

/* don't actually copy anything, just output the address + len and advance offset */
int sshbuf_get_string_direct(struct sshbuf *buf, const u_char **stringstart_ptr, size_t *stringlength_ptr)
{
	size_t len;
	const u_char *p;
	int r;

    /* reset targets */
	if (stringstart_ptr != NULL) *stringstart_ptr = NULL;
	if (stringlength_ptr != NULL) *stringlength_ptr = 0;

    /* get start and length of next string in buffer */
	if ((r = sshbuf_peek_string_direct(buf, &p, &len)) < 0)	return r;

	if (stringstart_ptr != NULL) *stringstart_ptr = p;
	if (stringlength_ptr != NULL) *stringlength_ptr = len;

	/* advance offset variable in buffer until after this blob */
    if (sshbuf_consume(buf, len + 4) != 0) {
		/* Shouldn't happen */
		SSHBUF_DBG(("SSH_ERR_INTERNAL_ERROR"));
		SSHBUF_ABORT();
		return SSH_ERR_INTERNAL_ERROR;
	}

	return 0;
}

/* read length of next string from next 4 bytes of buffer; no copying or offsetting */
int sshbuf_peek_string_direct(const struct sshbuf *buf, const u_char **stringstart_ptr, size_t *length_out_ptr)
{
	u_int32_t len;
	const u_char *pointer = sshbuf_ptr(buf);

    /* reset targets */
	if (stringstart_ptr != NULL) *stringstart_ptr = NULL;
	if (length_out_ptr != NULL) *length_out_ptr = 0;
	
    /* buffer length under 4 */
    if (sshbuf_len(buf) < 4) {
		SSHBUF_DBG(("SSH_ERR_MESSAGE_INCOMPLETE"));
		return SSH_ERR_MESSAGE_INCOMPLETE; }
	
    /* 32 length bitpattern of u_chars = p[0] | p[1] | ... */
    len = PEEK_U32(pointer);
	
    /* check length sanity */
    /* SSHBUF_SIZE_MAX is 128 MiB */
    if (len > SSHBUF_SIZE_MAX - 4) {
        /* length + string shall not be more than MAX */
		SSHBUF_DBG(("SSH_ERR_STRING_TOO_LARGE"));
		return SSH_ERR_STRING_TOO_LARGE; }
    if (sshbuf_len(buf) - 4 < len) {
        /* remaining buffer length shall be larger than string length */
		SSHBUF_DBG(("SSH_ERR_MESSAGE_INCOMPLETE"));
		return SSH_ERR_MESSAGE_INCOMPLETE; }

    /* write start addr and length */
	if (stringstart_ptr != NULL) *stringstart_ptr = pointer + 4;
	if (length_out_ptr != NULL) *length_out_ptr = len;
	
    return 0;
}

/* get next string in buffer */
int sshbuf_get_cstring(struct sshbuf *buf, char **target_ptr, size_t *length_out_ptr)
                      /* decoded           ciphername ..       NULL  */ 
{
	size_t strlen;
	const u_char *p, *z;
	int r;

    /* resetting targets */
	if (target_ptr != NULL) *target_ptr = NULL;
	if (length_out_ptr != NULL) *length_out_ptr = 0;

    /* get startaddr + length of next blob */
	if ((r = sshbuf_peek_string_direct(buf, &p, &strlen)) != 0) return r;
	
    /* Allow a \0 only at the end of the string */
	if (strlen > 0 && (z = memchr(p , '\0', strlen)) != NULL && z < p + strlen - 1) {
		SSHBUF_DBG(("SSH_ERR_INVALID_FORMAT"));	return SSH_ERR_INVALID_FORMAT; }
	
    /* skips over the next string (the one we 'found' above) */
    if ((r = sshbuf_skip_string(buf)) != 0)	return -1;
        /* = sshbuf_get_string_direct(buf, NULL, NULL) */
	
    /* if target pointer given */
    if (target_ptr != NULL) {
        /* allocate some new mem there */
		if ((*target_ptr = malloc(strlen + 1)) == NULL) {
			SSHBUF_DBG(("SSH_ERR_ALLOC_FAIL"));	return SSH_ERR_ALLOC_FAIL; }
        /* .. and copy found string there */
		if (strlen != 0) memcpy(*target_ptr, p, strlen);
		(*target_ptr)[strlen] = '\0'; /* terminate with nullchar */
	}
	/* if length pointer given copy found length there */
    if (length_out_ptr != NULL) *length_out_ptr = (size_t)strlen;
	return 0;
}

#endif /* DO NOT INCLUDE */