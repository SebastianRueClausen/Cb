#ifndef ERR_H
#define ERR_H

#include <stdint.h>
#include <stdint.h>

/* Error handeling */
typedef struct err_location
{
	char*						filename;
	uint32_t					line;
	uint32_t					col;
}
err_location_t;

void
fatal_error(const char *fmt, ...);

void
syntax_error(err_location_t loc, const char *fmt, ...);

void
syntax_warning(err_location_t loc, const char *fmt, ...);

#endif
