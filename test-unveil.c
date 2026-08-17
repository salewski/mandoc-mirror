#include <unistd.h>

int
main(void)
{
	return !!unveil(NULL, NULL);
}
