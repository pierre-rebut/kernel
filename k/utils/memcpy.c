#include <string.h>

void *memcpy(void *dest, const void *src, u32 n)
{
	const char *s = src;
	char *d = dest;

	for (u32 i = 0; i < n; i++)
		*d++ = *s++;

	return dest;
}
