#ifndef TYPE_H
#define TYPE_H

#include "def.h"

#include <stdint.h>

/* max values */
#define TYPE_UCHAR_MAX		255
#define TYPE_USHORT_MAX		65535
#define TYPE_UINT_MAX		4294967295
#define TYPE_ULONG_MAX		18446744073709551615

enum type_spec
{
	TYPE_SPEC_CONST				= 1 << 0,
	TYPE_SPEC_EXTERN			= 1 << 1,
	TYPE_SPEC_LONG				= 1 << 2,
	TYPE_SPEC_REGISTER			= 1 << 3,
	TYPE_SPEC_SIGNED			= 1 << 4,
	TYPE_SPEC_STATIC			= 1 << 5,
	TYPE_SPEC_UNSIGNED			= 1 << 6,
	TYPE_SPEC_VOLATILE			= 1 << 7,
	TYPE_SPEC_SHORT				= 1 << 8
};

enum type_prim
{
	TYPE_PRIM_NONE,
	TYPE_PRIM_VOID,
	TYPE_PRIM_CHAR,
	TYPE_PRIM_INT,
	TYPE_PRIM_FLOAT,
	TYPE_PRIM_DOUBLE,
	_TYPE_PRIM_COUNT
};

enum type_compat
{
	TYPE_COMPAT_INCOMPAT,
	TYPE_COMPAT_COMPAT,
	TYPE_COMPAT_WIDEN_LEFT,
	TYPE_COMPAT_WIDEN_RIGHT
};


struct type_info
{
	enum type_spec				spec;
	enum type_prim				prim;
	uint32_t					indirection;
};

static const struct type_info NULL_TYPE_INFO =
{
	.prim						= TYPE_PRIM_NONE,
	.spec						= 0,
	.indirection				= 0
};

enum type_literal_type
{
	TYPE_LIT_INT,
	TYPE_LIT_FLOAT,
	TYPE_LIT_STRING
};

union type_literal_value
{
	double						val_float;
	uint64_t					val_int;

	struct {
								uint32_t size;
								const char* data;
	} str;
};

void
type_check_conflicts(struct type_info type, struct err_location *err_loc);

size_t
type_get_width(const struct type_info type);

enum type_compat
type_compat(struct type_info left, struct type_info right);

struct type_info
type_combine_suffix_and_lit(struct type_info suffix, struct type_info literal);

void
type_print(const struct type_info type);

struct type_info
type_deduct_from_literal(union type_literal_value lit, enum type_literal_type lit_type);

#endif
