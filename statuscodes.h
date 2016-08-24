/* collection of all following definitions */
#define STATUSCODES(fn) MISC_STATUS(fn), BUFFER_STATUS(fn), FILEIO_STATUS(fn)

/* general statuscodes */
#define MISC_STATUS(fn) \
    fn( SUCCESS,        Everything is fine.                     ),\
    fn( FAILURE,        An unspecified error occured.           ),\
    fn( USAGE,          Wrong usage of program.                 ),\
    fn( NULLPOINTER,    Nullpointer in a non-optional argument. )

/* statuscodes for buffer.h */
#define BUFFER_STATUS(fn) \
    fn( BUFFER_INTERNAL_ERROR,          Some unspecified internal error occured in a buffer function.   ),\
    fn( BUFFER_LENGTH_OVER_MAXIMUM,     A resizing operation would exceed the specified maximum size.   ),\
    fn( BUFFER_MEMCPY_FAIL,             A memcpy failed in a buffer function.                           ),\
    fn( BUFFER_ALLOCATION_FAILED,       Failed creating a new buffer structure.                         ),\
    fn( BUFFER_MALLOC_FAILED,           A buffer function failed to allocate additional memory.         ),\
    fn( BUFFER_REALLOC_FAILED,          A buffer resize/realloc failed.                                 ),\
    fn( BUFFER_OFFSET_TOO_LARGE,        Requested offset goes beyond the current size of a buffer.      ),\
    fn( BUFFER_INCOMPLETE_MESSAGE,      Message size does not match with the encoded length.            ),\
    fn( BUFFER_INVALID_FORMAT,          A function received ill-formatted data.                         )

/* statuscodes for fileops.h */
#define FILEIO_STATUS(fn) \
    fn( FILEOPS_CANNOT_OPEN_READING,    Cannot open file for reading.               ),\
    fn( FILEOPS_CANNOT_OPEN_WRITING,    Cannot open file for writing.               ),\
    fn( FILEOPS_IOERROR,                General Input/Output error occured.         ),\
    fn( FILEOPS_INCOMPLETE_WRITE,       Incomplete write, possibly corrupt data.    ),\
    fn( FILEOPS_ALLOCATION_FAIL,        New buffer allocation failed.               )
