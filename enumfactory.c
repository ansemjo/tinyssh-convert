#include <stdio.h>

#define STATUS_LIST \
    E( SUCCESS, This is not an error. ), \
    E( INVALID, Something went wrong. ), \
    E( ERRM,    Just another error.   )

#define STATUS_STRUCT int code; const char *label; const char *description;

/* build the enum list */
#define E(enum, description) enum
enum status_enums { STATUS_LIST, STATUSMAX };

/* build the description array */
#undef E
#define E(enum, description) { enum, #enum, #description }
static const struct { STATUS_STRUCT } const status[] = { STATUS_LIST };


int main (void)
{
    for (int e = SUCCESS; e < STATUSMAX; e++)
        printf("error [%d]%s has description: \"%s\"\n",
            status[e].code, status[e].label, status[e].description);

    return 0;

}