#include "def.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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

	exit(0);
}

/* prints a warning and the code location */
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


/* same as malloc, and should work with every data type */
#define MEM_POOL_ALIGN 8

static void*
align_ptr(const void *ptr)
{
	intptr_t address = (intptr_t)ptr;
	address += (-address) & (MEM_POOL_ALIGN - 1);
	return (void*)address;
}

static struct mem_block*
create_block(size_t block_size)
{
	struct mem_block *block;

	block = c_malloc(block_size + sizeof(struct mem_block));

	block->end = (char*)block + sizeof(struct mem_block) + block_size;
	block->start = (char*)block + sizeof(struct mem_block);
	block->top = block->start;

	return block;
}

/* creates a new memory pool, and allocates one chunk */
struct mem_pool
mem_pool_create(size_t block_size)
{
	struct mem_pool pool;

	pool.block_size = block_size;
	pool.blocks = create_block(block_size);

	return pool;
}

/* deallocates all chunks in a memory pool */
void
mem_pool_destroy(struct mem_pool *pool)
{
	struct mem_block *block = pool->blocks;

	while (block) {
		c_free(block);
		block = block->prev;
	}
}

/* allocates space, and allocates a new chunk id required */
void*
mem_pool_alloc(struct mem_pool *pool, size_t size)
{
	void *ptr;
	struct mem_block *new_block;

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
