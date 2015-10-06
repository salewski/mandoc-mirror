#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>

int
main(void)
{
	size_t sz;
	fclose(stdin);
	return fgetln(stdin, &sz) != NULL;
}
