#include "gen.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void fatal_error(const char* fmt, ...)
{
	va_list args; 
	va_start(args, fmt);

	vprintf(fmt, args);

	exit(0);
}

void* c_malloc(size_t size)
{
	void* ptr = malloc(size);

	if (!ptr)
		fatal_error("Error: Out of memory");

	return ptr;
}

void* c_realloc(void* ptr, size_t size)
{
	ptr = realloc(ptr, size);

	if (!ptr)
		fatal_error("Error: Out of memory");

	return ptr;
}

void c_free(void* ptr)
{
	free(ptr);
}
