/* collection of all following definitions */
#define STATUSCODES(fn) MISC_STATUS(fn), BUFFER_STATUS(fn), FILEIO_STATUS(fn), OPENSSH_KEY_STATUS(fn), OPENSSH_PARSE_STATUS(fn)

/* general statuscodes */
#define MISC_STATUS(fn) \
    fn( SUCCESS,                Everything is fine.                                 ),\
    fn( FAILURE,                An unspecified error occured.                       ),\
    fn( ERR_USAGE,              Wrong usage of program.                             ),\
    fn( ERR_NULLPTR,            Nullpointer in a non-optional argument.             ),\
    fn( ERR_BAD_ARGUMENT,       A given argument could not be processed correctly.  ),\
    fn( ERR_BAD_USER_INPUT,     A user-supplied value could not be processed.       )

/* statuscodes for buffer.h */
#define BUFFER_STATUS(fn) \
    fn( BUFFER_INTERNAL_ERROR,          Internal error occured in a buffer function.                    ),\
    fn( BUFFER_LENGTH_OVER_MAXIMUM,     A resizing operation would exceed the specified maximum size.   ),\
    fn( BUFFER_MEMCPY_FAIL,             A memcpy failed in a buffer function.                           ),\
    fn( BUFFER_ALLOCATION_FAILED,       Failed creating a new buffer structure.                         ),\
    fn( BUFFER_MALLOC_FAILED,           A buffer function failed to allocate memory.                    ),\
    fn( BUFFER_REALLOC_FAILED,          A buffer resize/realloc failed.                                 ),\
    fn( BUFFER_OFFSET_TOO_LARGE,        Requested offset goes beyond the current size of a buffer.      ),\
    fn( BUFFER_END_OF_BUF,              Reached end of buffer when trying to read.                      ),\
    fn( BUFFER_INCOMPLETE_MESSAGE,      Message size does not match with the encoded length.            ),\
    fn( BUFFER_INVALID_FORMAT,          A function received ill-formatted data.                         )

/* statuscodes for fileio.h */
#define FILEIO_STATUS(fn) \
    fn( FILEIO_CANNOT_OPEN_READING,     Cannot open file for reading.               ),\
    fn( FILEIO_CANNOT_OPEN_WRITING,     Cannot open file for writing.               ),\
    fn( FILEIO_IOERROR,                 General Input/Output error occured.         ),\
    fn( FILEIO_INCOMPLETE_WRITE,        Incomplete write, possibly corrupt data.    )

/* statuscodes for openssh-key.h */
#define OPENSSH_KEY_STATUS(fn) \
    fn( OPENSSH_KEY_INCOMPATIBLE,       Tried to call a function for a different keytype.   ),\
    fn( OPENSSH_KEY_UNKNOWN_KEYTYPE,    The keytype is unknown or unspecified.              ),\
    fn( OPENSSH_KEY_ALLOCATION_FAILURE, Failed creating a new key structure.                )

/* statuscodes for openssh-parse.h */
#define OPENSSH_PARSE_STATUS(fn) \
    fn( OPENSSH_PARSE_INVALID_FORMAT,               The observed key format did not match the specification.    ),\
    fn( OPENSSH_PARSE_INVALID_PRIVATE_FORMAT,       The private key was malformed.                              ),\
    fn( OPENSSH_PARSE_UNSUPPORTED_CIPHER,           This encryption cipher is not supported.                    ),\
    fn( OPENSSH_PARSE_UNSUPPORTED_KDF,              This key derivation function is not supported.              ),\
    fn( OPENSSH_PARSE_UNSUPPORTED_MULTIPLEKEYS,     Multiple keys in one file are not supported.                ),\
    fn( OPENSSH_PARSE_UNSUPPORTED_KEY_TYPE,         This keytype is not supported for parsing.                  ),\
    fn( OPENSSH_PARSE_INTERNAL_ERROR,               Internal error occured in a parsing function.               )
