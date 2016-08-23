#ifndef _headerguard_errors_h_
#define _headerguard_errors_h_

#include <stdio.h>

/* define general errorcodes and descriptions */
#define ERROR_LIST(fn) \
    fn(SUCCESS, Everything is fine. ),\
    fn(FAILURE, An unspecified error occured. ),\
    fn(USAGE,   Wrong usage of program. )

#define AS_ENUM(enum, description) enum
#define AS_STRUCT(enum, description) { enum, #enum, #description }

/*  define your error codes and descriptions around the project
 *  with structures like the ERROR_LIST(fn) above, #include those
 *  header files here and include the lists in the following define:
 */
#define ERROR_COLLECTION(fn) ERROR_LIST(fn)

/* this will build an enum with all errorcodes .. */
enum errorcodes { ERROR_COLLECTION(AS_ENUM), ERRORMAX };

/* and a struct with labels and descriptions, indexable by errorcode */
static const struct {
    int code;
    const char *label;
    const char *reason;
} const errorindex[] = { ERROR_COLLECTION(AS_STRUCT) };

/* shorthands to access properties of errorcode e */ 
#define elabel(e)  errorindex[e].label
#define ereason(e) errorindex[e].reason

/* printf to stderr */
#define eprintf(...) fprintf (stderr, __VA_ARGS__)

/* fatally exit with an error message */
#define fatal(e, ...) do { eprintf(__VA_ARGS__); if ((e % 256) == 0) exit(FAILURE); else exit(e); } while(0)

/* print usage message */
#ifdef USAGE_MESSAGE
# define usage() fatal(USAGE, "%s\n", USAGE_MESSAGE)
#else
# define usage() fatal(USAGE, "%s: %s\n", elabel(USAGE), ereason(USAGE))
#endif

/* early return, e.g. if some condition failed, but do cleanup first! */
#define cleanreturn(status) do { e = (status); goto cleanup; } while (0)

#endif