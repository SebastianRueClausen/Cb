#ifndef _TYPE_
#define _TYPE_

#include "err.h"
#include "f_def.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum type_spec
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

} type_spec_t;


typedef enum type_prim
{
	TYPE_PRIM_NONE,
	TYPE_PRIM_VOID,
	TYPE_PRIM_CHAR,
	TYPE_PRIM_INT,
	TYPE_PRIM_FLOAT,
	TYPE_PRIM_DOUBLE,
	_TYPE_PRIM_COUNT

} type_prim_t;


typedef enum type_compat
{
	TYPE_COMPAT_INCOMPAT,
	TYPE_COMPAT_COMPAT,
	TYPE_COMPAT_PROMOTE_LEFT,
	TYPE_COMPAT_PROMOTE_RIGHT

} type_compat_t;


typedef struct type_info
{
	type_spec_t					spec;
	type_prim_t					prim;
	uint32_t					indirection;

} type_info_t;


static const type_info_t		NULL_TYPE_INFO =
{
	.prim						= TYPE_PRIM_NONE,
	.spec						= 0,
	.indirection				= 0
};


void			type_check_validity(type_info_t *type, struct err_location *err_loc);
size_t			type_get_width(const type_info_t type);
type_compat_t	type_compat(type_info_t left, type_info_t right);
void			type_adapt_to_suffix(literal_t *literal, suffix_flags_t flags, err_location_t err_loc);
bool			type_compare(type_info_t a, type_info_t b);
type_info_t		type_from_literal(literal_t literal);

void			type_print(const type_info_t type);


#endif
