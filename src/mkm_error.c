#include <stdarg.h>
#include <stdio.h>

#include "mkm_error.h"

void	
mkm_error(
	const char* format,
	...)
{
	char buffer[1024];

	{
		va_list list;
		va_start(list, format);

		int n = vsnprintf(buffer, sizeof(buffer), format, list);
		if (n < 0)
			buffer[0] = '\0';

		va_end(list);
	}

	fprintf(stderr, "Error: %s\n", buffer);

	exit(1);
}

