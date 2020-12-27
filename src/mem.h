#ifndef MEM_H
#define MEM_H

#include <stddef.h>

/* memory */
void*
c_malloc(size_t size);

void*
c_realloc(void *ptr, size_t size);

void
c_free(void *ptr);


/* pool allocator */
typedef struct mem_block
{
	char						*start;	
	char						*top;
	char						*end;
	
	struct mem_block			*prev;
}
mem_block_t;

typedef struct mem_pool
{
	size_t						block_size;
	struct mem_block			*blocks;
}
mem_pool_t;

mem_pool_t
mem_pool_create(size_t block_size);

void
mem_pool_destroy(mem_pool_t *pool);

void*
mem_pool_alloc(mem_pool_t *pool, size_t size);

#endif
