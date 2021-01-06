#include "mem.h"

#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#include "err.h"

/* same as malloc, and should work with every data type */
#define MEM_POOL_ALIGN 8

static void *
align_ptr(const void *ptr)
{
    intptr_t address = (intptr_t)ptr;
    address += (-address) & (MEM_POOL_ALIGN - 1);
    return (void *)address;
}

static mem_block_t *
create_block(size_t block_size)
{
    mem_block_t *block;

    block = c_malloc(block_size + sizeof(mem_block_t));

    block->end   = (byte_t *)block + sizeof(mem_block_t) + block_size;
    block->start = (byte_t *)block + sizeof(mem_block_t);
    block->top   = block->start;
	block->next  = NULL;

    return block;
}

/* creates a new memory pool, and allocates one chunk */
mem_pool_t
mem_pool_create(size_t block_size)
{
    mem_pool_t pool;

    pool.block_size = block_size;
    pool.first      = create_block(block_size);
	pool.last		= pool.first;

    return pool;
}

/* deallocates all chunks in a memory pool */
void
mem_pool_destroy(mem_pool_t *pool)
{
    mem_block_t *block = pool->first;

    while (block)
    {
        c_free(block);
        block = block->next;
    }

	pool->first = NULL;
	pool->last  = NULL;
}

/* allocates space, and allocates a new chunk id required */
void *
mem_pool_alloc(mem_pool_t *pool, size_t size)
{
    byte_t		*ptr;
    mem_block_t *new_block;
	size_t		 block_size;

    assert(size <= pool->block_size);

    ptr = align_ptr(pool->last->top);

    /* if there isn't enough room in the current block */
    if (ptr + size > pool->last->end)
    {
		block_size = pool->block_size;
		
		/* we allocate a new block of large enough */
		while (block_size < size)
		{
			block_size += pool->block_size;
		}

        new_block        = create_block(block_size);
		pool->last->next = new_block;
        pool->last		 = new_block;

        ptr              = align_ptr(pool->last->top);
        pool->last->top	 = ptr + size;
    }
	else
	{
		pool->last->top = ptr + size;
	}

    return ptr;
}

static bool
is_in_block(mem_block_t *block, byte_t *ptr)
{
	if (ptr > block->start && ptr < block->end)
	{
		return true;	
	}

	return false;
}

#include <stdio.h>

/* free to the point of the pointer */
void
mem_pool_free(mem_pool_t *pool, void *ptr)
{
	mem_block_t *block;
	mem_block_t *tmp;

	block = pool->first;

	/* find the block the ptr is in */
	while (block)
	{
		if (is_in_block(block, ptr))
		{
			block->top = ptr;
			break;
		}

		block = block->next;
	}

	/* if it isnt the last block we remove all following blocks */
	if (block->next)
	{
		block = block->next;

		/* free the blocks after the current block */
		while (block)
		{
			tmp = block->next;

			c_free(block);

			block = tmp;
		}
	}

}

/* free all chunks besides the first */
void
mem_pool_free_all(mem_pool_t *pool)
{
	mem_block_t *tmp;
	mem_block_t *block = pool->first->next;

	while (block)
	{
		tmp = block->next;
		c_free(block);
		block = tmp;
	}

	pool->first->next = NULL;
	pool->first->top  = pool->first->start;
}

/* prints an error if malloc fails */
void *
c_malloc(size_t size)
{
    void *ptr = malloc(size);

    if (!ptr)
    {
        fatal_error("out of memory");
    }

    return ptr;
}

/* prints an error if realloc fails */
void *
c_realloc(void *ptr, size_t size)
{
    ptr = realloc(ptr, size);

    if (!ptr)
    {
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
