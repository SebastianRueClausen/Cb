#include "err.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/* just prints the error message and exits the program */
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

/* prints the error and a code location and exits the program*/
void
syntax_error(err_location_t loc, const char *fmt, ...)
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

	exit(0);
}

/* prints a warning and the code location */
void
syntax_warning(err_location_t loc, const char *fmt, ...)
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


