#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdint.h>

struct err_location
{
	char* filename;
	uint32_t line;
	uint32_t col;
};

void fatal_error(const char* fmt, ...);
void syntax_error(struct err_location loc, const char* fmt, ...);
void syntax_warning(struct err_location loc, const char* fmt, ...);

void* c_malloc(size_t size);
void* c_realloc(void* ptr, size_t size);
void  c_free(void* ptr);

#endif

