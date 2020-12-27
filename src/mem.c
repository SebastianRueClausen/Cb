#include "mem.h"

#include <stdint.h>
#include <assert.h>
#include <stdlib.h>

#include "err.h"

/* same as malloc, and should work with every data type */
#define MEM_POOL_ALIGN 8

static void*
align_ptr(const void *ptr)
{
	intptr_t address = (intptr_t)ptr;
	address += (-address) & (MEM_POOL_ALIGN - 1);
	return (void*)address;
}

static mem_block_t*
create_block(size_t block_size)
{
	mem_block_t *block;

	block = c_malloc(block_size + sizeof(mem_block_t));

	block->end = (char*)block + sizeof(mem_block_t) + block_size;
	block->start = (char*)block + sizeof(mem_block_t);
	block->top = block->start;

	return block;
}

/* creates a new memory pool, and allocates one chunk */
mem_pool_t
mem_pool_create(size_t block_size)
{
	mem_pool_t pool;

	pool.block_size = block_size;
	pool.blocks = create_block(block_size);

	return pool;
}

/* deallocates all chunks in a memory pool */
void
mem_pool_destroy(mem_pool_t *pool)
{
	mem_block_t *block = pool->blocks;

	while (block) {
		c_free(block);
		block = block->prev;
	}
}

/* allocates space, and allocates a new chunk id required */
void*
mem_pool_alloc(mem_pool_t *pool, size_t size)
{
	void *ptr;
	mem_block_t *new_block;

	assert(size <= pool->block_size);
	
	ptr = align_ptr(pool->blocks->top);
	pool->blocks->top = ptr + size;

	/* if there isn't enough room in the current block */
	if (pool->blocks->top > pool->blocks->end) {
		new_block = create_block(pool->block_size);
		new_block->prev = pool->blocks;
		pool->blocks = new_block;

		ptr = align_ptr(pool->blocks->top);
		pool->blocks->top = ptr + size;
	}

	return ptr;
}

/* prints an error if malloc fails */
void *
c_malloc(size_t size)
{
	void *ptr = malloc(size);

	if (!ptr) {
		fatal_error("out of memory");
	}

	return ptr;
}

/* prints an error if realloc fails */
void *
c_realloc(void *ptr, size_t size)
{
	ptr = realloc(ptr, size);

	if (!ptr) {
		fatal_error("realloc failed");
	}

	return ptr;
}

/* internal free, may be used for debug later */
void
c_free(void *ptr)
{
	free(ptr);
}
