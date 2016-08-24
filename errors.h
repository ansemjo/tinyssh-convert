/*
 * This file is governed by Licenses which are listed in
 * the LICENSE file, which shall be included in all copies
 * and redistributions of this project.
 */

#ifndef _headerguard_errors_h_
#define _headerguard_errors_h_

#include <stdio.h>

/* statuscode definitions */
#include "statuscodes.h"

/* function to build enums and structs from tuples */
#define AS_ENUM(enum, ...) enum
#define AS_STRUCT(enum, ...) { enum, #enum, #__VA_ARGS__ }

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
# define usage() fatal(ERR_USAGE, "%s\n", USAGE_MESSAGE)
#else
# define usage() fatal(ERR_USAGE, "%s: %s\n", elabel(ERR_USAGE), ereason(ERR_USAGE))
#endif

/* early return, e.g. if some condition failed, but do cleanup first! */
#define cleanreturn(status) do { e = (status); goto cleanup; } while (0)

#endif