#include <string.h>

int strncmp(const char *s1, const char *s2, u32 n)
{
	for (u32 i = 0; i < n; ++i)
		if (s1[i] == '\0' || s1[i] != s2[i])
			return s1[i] - s2[i];

	return 0;
}
