#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

void fatal_error(const char* fmt, ...);

void* c_malloc(size_t size);
void* c_realloc(void* ptr, size_t size);
void  c_free(void* ptr);

#endif

