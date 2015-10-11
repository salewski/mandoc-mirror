#include <string.h>

extern char *__progname;

int
main(void)
{
	return !!strcmp(__progname, "test-progname");
}
