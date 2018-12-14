#include <string.h>
#include <k/types.h>

u32 strlen(const char *s)
{
	const char *p = s;

	while (*p)
		p++;

	return (p - s);
}
