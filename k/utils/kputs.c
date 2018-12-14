#include <string.h>
#include "io/terminal.h"

int kputs(const char *s)
{
	return writeStringTerminal(s, strlen(s));
}
