#ifndef MEM_H
#define MEM_H

#include <stddef.h>

/* memory */
void *c_malloc(size_t size);

void *c_realloc(void *ptr, size_t size);

void c_free(void *ptr);


/* pool allocator */
typedef char             byte_t;
typedef struct mem_block mem_block_t;

typedef struct mem_block
{
    byte_t *start;
    byte_t *top;
    byte_t *end;

    mem_block_t *next;

} mem_block_t;

typedef struct mem_pool
{
    size_t       block_size;
    mem_block_t *first;
    mem_block_t *last;

} mem_pool_t;

mem_pool_t mem_pool_create(size_t block_size);
void       mem_pool_destroy(mem_pool_t *pool);
void *     mem_pool_alloc(mem_pool_t *pool, size_t size);
void       mem_pool_free(mem_pool_t *pool, void *ptr);
void	   mem_pool_free_all(mem_pool_t *pool);

#endif
