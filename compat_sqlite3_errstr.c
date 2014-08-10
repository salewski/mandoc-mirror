#include "config.h"

#ifdef HAVE_SQLITE3_ERRSTR

int dummy;

#else

const char *
sqlite3_errstr(int rc)
{

	return(rc ? "unknown error" : "not an error");
}

#endif
