#ifndef	_F_DEF_
#define _F_DEF_

#include <stdint.h>

#define MAX_SUFFIX_LEN 2

typedef enum suffix_flags
{
	SUFFIX_UNSIGNED = 1 << 0,
	SUFFIX_LONG		= 1 << 1,
	SUFFIX_FLOAT	= 1 << 2,

} suffix_flags_t;


typedef union value
{
	double				_float;
	uint64_t			_int;

	struct {
						uint32_t size;
						const char* data;
	} str;

} value_t;


typedef struct literal
{
	/* perhaps not the best way to represent the type of literal */
	enum {
		LITERAL_TYPE_INT,
		LITERAL_TYPE_LONG,
		LITERAL_TYPE_DOUBLE,
		LITERAL_TYPE_STR

		/* becomes int */
		/* LITERAL_TYPE_CHAR */	

	} type;

	value_t					value;

} literal_t;

#endif
