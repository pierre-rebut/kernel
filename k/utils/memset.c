#include <string.h>

void *memset(void *s, int c, u32 n)
{
	char *p = s;

	for (u32 i = 0; i < n; ++i)
		p[i] = c;

	return s;
}
