#ifndef VEC_TYPE
	#error "VEC_TYPE not defined before including vec.h"
#endif

#include "template.h"

/* create signature */
#define VEC_SIGNATURE TEMPLATE_SIGNATURE(vec, VEC_TYPE)

typedef struct
{
	VEC_TYPE *data;
	size_t size;	
	size_t capacity;
}
VEC_SIGNATURE;

static inline VEC_SIGNATURE
TEMPLATE_SIGNATURE(VEC_SIGNATURE, create)(size_t capacity)
{
	const VEC_SIGNATURE vec =
	{
		.data		= malloc(sizeof(VEC_TYPE) * capacity),
		.capacity	= capacity,
		.size		= 0
	};

	return vec;	
}

static inline void
TEMPLATE_SIGNATURE(VEC_SIGNATURE, destroy)(VEC_SIGNATURE *vec)
{
	if (vec->data) {
		free(vec->data);
	}
}

static inline void
TEMPLATE_SIGNATURE(VEC_SIGNATURE, reserve)(VEC_SIGNATURE *vec, size_t capacity)
{
	if (vec->capacity != capacity) {
		vec->data = realloc(vec->data, capacity * sizeof(VEC_TYPE));
		
		if (capacity < vec->size) {
			vec->size = capacity;
		}

		vec->capacity = capacity;
	}
}

static inline void
TEMPLATE_SIGNATURE(VEC_SIGNATURE, resize)(VEC_SIGNATURE *vec, size_t size)
{
	/* only allow downsizeing for now */
	assert(size <= vec->size);

	vec->size = size;
}

static inline void
TEMPLATE_SIGNATURE(VEC_SIGNATURE, push)(VEC_SIGNATURE *vec, VEC_TYPE elem)
{
	/* if we are out of capacity */
	if (vec->size == vec->capacity) {
		/* we double the size for now */
		if (vec->capacity == 0) {
			TEMPLATE_SIGNATURE(VEC_SIGNATURE, reserve)(vec, 1);
		} else {
			TEMPLATE_SIGNATURE(VEC_SIGNATURE, reserve)(vec, vec->capacity * 2);
		}
	}

	vec->data[vec->size] = elem;
	++vec->size;	
}

static inline void
TEMPLATE_SIGNATURE(VEC_SIGNATURE, pop)(VEC_SIGNATURE *vec)
{
	if (vec->size != 0) {
		--vec->size;
	}
}

static inline VEC_TYPE
TEMPLATE_SIGNATURE(VEC_SIGNATURE, top)(VEC_SIGNATURE *vec)
{
	return vec->data[vec->size - 1];
}

static inline VEC_TYPE*
TEMPLATE_SIGNATURE(VEC_SIGNATURE, top_ptr)(VEC_SIGNATURE *vec)
{
	return &vec->data[vec->size - 1];
}

