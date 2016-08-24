#ifndef _headerguard_errors_h_
#define _headerguard_errors_h_

#include <stdio.h>

/* statuscode definitions */
#include "statuscodes.h"

/* function to build enums and structs from tuples */
#define AS_ENUM(enum, description) enum
#define AS_STRUCT(enum, description) { enum, #enum, #description }

/* this will build an enum with all statuscodes .. */
enum errorcodes { STATUSCODES(AS_ENUM), STATUSMAX };

/* and a struct with labels and descriptions, indexable by errorcode */
static const struct {
    int code;
    const char *label;
    const char *reason;
} const errorindex[] = { STATUSCODES(AS_STRUCT) };

/* shorthands to access properties of errorcode e */ 
#define elabel(e)  errorindex[e].label
#define ereason(e) errorindex[e].reason

/* printf to stderr */
#define eprintf(...) fprintf (stderr, __VA_ARGS__)

/* fatally exit with an error message */
#define fatal(e, ...) do { eprintf(__VA_ARGS__); if ((e % 256) == 0) exit(FAILURE); else exit(e); } while(0)
#define fatale(e) fatal(e, "%s\n", ereason(e))

/* print usage message */
#ifdef USAGE_MESSAGE
# define usage() fatal(USAGE, "%s\n", USAGE_MESSAGE)
#else
# define usage() fatal(USAGE, "%s: %s\n", elabel(USAGE), ereason(USAGE))
#endif

/* early return, e.g. if some condition failed, but do cleanup first! */
#define cleanreturn(status) do { e = (status); goto cleanup; } while (0)

#endif