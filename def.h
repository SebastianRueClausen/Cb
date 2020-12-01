#ifndef DEF_H
#define DEF_H

#include <stddef.h>
#include <stdint.h>

/*
 * Misc structures and helper functions
 */

/* memory */
void*
c_malloc(size_t size);

void*
c_realloc(void *ptr, size_t size);

void
c_free(void *ptr);


/* memory pool allocator */
struct mem_block
{
	char						*start;	
	char						*top;
	char						*end;
	
	struct mem_block			*prev;
};

struct mem_pool
{
	size_t						block_size;
	struct mem_block			*blocks;
};

struct mem_pool
mem_pool_create(size_t block_size);

void
mem_pool_destroy(struct mem_pool *pool);

void*
mem_pool_alloc(struct mem_pool *pool, size_t size);

/* Error handeling */
struct err_location
{
	char*						filename;
	uint32_t					line;
	uint32_t					col;
};

void
fatal_error(const char *fmt, ...);

void
syntax_error(struct err_location loc, const char *fmt, ...);

void
syntax_warning(struct err_location loc, const char *fmt, ...);

#endif
