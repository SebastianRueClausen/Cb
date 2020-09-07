#include "def.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void
fatal_error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	printf("\x1B[31m"
		   "[Error]: "
		   "\x1B[0m");

	vprintf(fmt, args);

	printf("\n");

	exit(0);
}

void
syntax_error(struct err_location loc, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	printf("%s: %u, %u: "
		   "\x1B[31m"
		   "[Syntax Error]: "
		   "\x1B[0m",
		   loc.filename, loc.line, loc.col);

	vprintf(fmt, args);

	printf("\n");

	// We exit for now, but should only stop the current file
	exit(0);
}

void
syntax_warning(struct err_location loc, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	printf("%s: %u, %u: "
		   "\x1B[33m"
		   "[Warning]: "
		   "\x1B[0m",
		   loc.filename, loc.line, loc.col);

	vprintf(fmt, args);

	printf("\n");
}

void *
c_malloc(size_t size)
{
	void *ptr = malloc(size);

	if (!ptr)
		fatal_error("out of memory");

	return ptr;
}

void *
c_realloc(void *ptr, size_t size)
{
	ptr = realloc(ptr, size);

	if (!ptr)
		fatal_error("realloc failed");

	return ptr;
}

void
c_free(void *ptr)
{
	free(ptr);
}
