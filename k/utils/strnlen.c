#include <string.h>

u32 strnlen(const char *s, u32 maxlen)
{
	u32 i = 0;
	for (; i < maxlen; ++i)
		if (!s[i])
			return i;
	return i;
}
