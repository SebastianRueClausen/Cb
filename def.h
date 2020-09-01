#ifndef DEF_H
#define DEF_H

#include <stddef.h>
#include <stdint.h>

union const_value
{
	double				val_double;
	float				val_float;
	int64_t				val_int;

	struct {
						int size;
						const char* data;
	} str;

	int					id;
};
	
struct err_location
{
	char*				filename;
	uint32_t			line;
	uint32_t			col;
};

void*
c_malloc(size_t size);

void*
c_realloc(void* ptr, size_t size);

void
c_free(void* ptr);

void
fatal_error(const char* fmt, ...);

void
syntax_error(struct err_location loc, const char* fmt, ...);

void
syntax_warning(struct err_location loc, const char* fmt, ...);

#endif
