#include <stdio.h>

#define STATUS_LIST(fn) \
    fn( SUCCESS, This is not an error. ), \
    fn( INVALID, Something went wrong. ), \
    fn( ERRM,    Just another error.   )

#define STATUS_STRUCT int code; const char *label; const char *description;

#define   as_enum(enum, description)      enum
#define as_triple(enum, description)    { enum, #enum, #description }

enum status_codes { STATUS_LIST(as_enum), STATUSMAX };
static const struct { STATUS_STRUCT } const status[] = { STATUS_LIST(as_triple) };

int main (void)
{
    for (int e = SUCCESS; e < STATUSMAX; e++)
        printf("error [%d]%s has description: \"%s\"\n",
            status[e].code, status[e].label, status[e].description);

    return 0;

}