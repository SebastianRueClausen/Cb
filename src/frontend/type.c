#include "frontend.h"

#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

static void
deduct_prim(type_info_t *type, err_location_t *err_loc)
{
	assert(type->prim == TYPE_PRIM_NONE);

	if (type->spec & TYPE_SPEC_UNSIGNED) {
		type->prim = TYPE_PRIM_INT;
		return;
	}

	if (type->spec & TYPE_SPEC_SIGNED) {
		type->prim = TYPE_PRIM_INT;
		return;
	}

	if (type->spec & TYPE_SPEC_SHORT) {
		type->prim = TYPE_PRIM_INT;
		return;
	}

	if (type->spec & TYPE_SPEC_LONG) {
		type->prim = TYPE_PRIM_INT;
		return;
	}

	type_print(*type);

	syntax_error(*err_loc, "type specifier missing");			
}

/* check for conflicts of a type info */
void
type_check_validity(type_info_t *type, err_location_t *err_loc)
{
	switch (type->prim) {
		case TYPE_PRIM_FLOAT:
			if (type->spec & TYPE_SPEC_SIGNED || type->spec & TYPE_SPEC_UNSIGNED) {
				syntax_error(*err_loc, "type float can not be specified "
										"as signed or unsigned");
			} else if (type->spec & TYPE_SPEC_LONG) {
				syntax_error(*err_loc, "type float can not be specified as long");
			} else if (type->spec & TYPE_SPEC_SHORT) {
				syntax_error(*err_loc, "type float can not be specified as short");
			}
			break;

		case TYPE_PRIM_DOUBLE:	
			if (type->spec& TYPE_SPEC_SIGNED || type->spec& TYPE_SPEC_UNSIGNED) {
				syntax_error(*err_loc, "type double can not be specified "
										"as signed or unsigned");
			} else if (type->spec & TYPE_SPEC_LONG) {
				syntax_error(*err_loc, "type double can not be specified as long");
			} else if (type->spec & TYPE_SPEC_SHORT) {
				syntax_error(*err_loc, "type double can not be specified as short");
			}
			break;

		/* applies to all other types */
		default:
			/* check that there is a primitive */
			if (type->prim == TYPE_PRIM_NONE) {
				/* a type could be specified as fx 'unsigned i', which
				 * would still be valid even tough a prim isnt set */
				/* throws error if unable to */
				deduct_prim(type, err_loc);
			}
			
			if (type->spec & TYPE_SPEC_SIGNED && type->spec & TYPE_SPEC_UNSIGNED) {
				syntax_error(*err_loc, "type can not be specified as signed and unsigned");
			} else if (type->spec & TYPE_SPEC_LONG && type->spec & TYPE_SPEC_SHORT) {
				syntax_error(*err_loc, "type can not be specified as short and long");
			}
			break;
	}
}

/* this may ofcourse change dependent on architecture */
static uint32_t type_prim_width[_TYPE_PRIM_COUNT] =
{
	[TYPE_PRIM_NONE]				= 0,
	[TYPE_PRIM_VOID]				= 0,
	[TYPE_PRIM_CHAR]				= 1,
	[TYPE_PRIM_INT]					= 4,
	[TYPE_PRIM_FLOAT]				= 4,
	[TYPE_PRIM_DOUBLE]				= 8
};

size_t
type_get_width(const type_info_t type)
{
	if (type.indirection != 0) {
		return 8;
	}

	/* since we check that type specifiers doesnt conflict
	 * we should just be able to check if its short or long */
	if (type.spec & TYPE_SPEC_SHORT) {
		return 2;
	}

	if (type.spec & TYPE_SPEC_LONG) {
		return 8;
	}

	return type_prim_width[type.prim];
}

/* check the compat of two types */
type_compat_t
type_compat(type_info_t left, type_info_t right)
{
	uint32_t left_width, right_width;

	/* void is always incompatible */
	if (left.prim == TYPE_PRIM_VOID || right.prim == TYPE_PRIM_VOID) {
		return TYPE_COMPAT_INCOMPAT;
	}

	/* is they are the same they are compatible */
	if (left.prim == right.prim) {
		return TYPE_COMPAT_COMPAT;
	}

	left_width = type_get_width(left);
	right_width = type_get_width(right);

	if (left_width == right_width) {
		return TYPE_COMPAT_COMPAT;
	}

	/* @todo: we should check how much we should widen with */
	if (left_width > right_width) {
		return TYPE_COMPAT_PROMOTE_RIGHT;
	}

	if (left_width < right_width) {
		return TYPE_COMPAT_PROMOTE_LEFT;
	}

	return TYPE_COMPAT_COMPAT;
}

type_info_t
type_deduct_from_literal(type_literal_value_t lit,
						 type_literal_type_t lit_type)
{
	type_info_t type = NULL_TYPE_INFO;

	type.indirection = 0;
	type.spec = 0;

	switch (lit_type) {
		
		case TYPE_LIT_INT:
			/* we make it a u32 or u64 dependent on size */
			type.prim = TYPE_PRIM_INT;

			if (lit.val_int > UINT32_MAX) {
				type.spec |= TYPE_SPEC_LONG;
			}

			/* @todo: add check for overflow */

			/* always unsigned since a literal can't have a signed value */
			type.spec |= TYPE_SPEC_SIGNED;
			
			return type;

		case TYPE_LIT_FLOAT:

			/* we always make it a double */
			type.prim = TYPE_PRIM_DOUBLE;
			return type;

		case TYPE_LIT_STRING:
			type.prim = TYPE_PRIM_CHAR;
			/* should this be const?? */
			type.spec = TYPE_SPEC_CONST;
			type.indirection = 1;
			return type;

	}
}

/* should already be checked that the suffix and literal are compat, 
 * we just change the type of the literal so it matches the suffix */
type_info_t
type_adapt_to_suffix(type_info_t suffix, type_info_t literal,
					 err_location_t err_loc)
{

	switch (literal.prim) {

		/* we always set a literal as double if it's a floating point */
		case TYPE_PRIM_DOUBLE:

			if (suffix.spec & TYPE_SPEC_UNSIGNED) {
				syntax_error(err_loc, "invalid suffix 'unsigned' for "
									   "floating point literal");
			}

			if (suffix.spec & TYPE_SPEC_LONG) {
				syntax_error(err_loc, "invalid suffix 'long' for "
									    "floating point literal");
			}
			
			return literal;

		case TYPE_PRIM_INT:
			/* should always be unsigned */

			if (suffix.prim == TYPE_PRIM_DOUBLE) {
				syntax_error(err_loc, "invalid suffix 'float' for "
									   "integer literal");
			}

			if (suffix.spec & TYPE_SPEC_LONG) {
				literal.spec |= TYPE_SPEC_LONG;
			}

			return literal;

		default:
			assert(false);
		
	}
}

bool
type_compare(type_info_t a, type_info_t b)
{
	if (a.prim == b.prim && a.spec == b.spec &&
		a.indirection == b.indirection) {
		return true;		
	} else {
		return false;
	}
}

static const char* type_prim_str[] =
{
	"TYPE_PRIM_NONE",
	"TYPE_PRIM_VOID",
	"TYPE_PRIM_CHAR",
	"TYPE_PRIM_INT",
	"TYPE_PRIM_FLOAT",
	"TYPE_PRIM_DOUBLE",
	"_TYPE_PRIM_COUNT"
};

static const char* type_spec_str[] =
{
	"TYPE_SPEC_CONST",
	"TYPE_SPEC_EXTERN",
	"TYPE_SPEC_LONG",
	"TYPE_SPEC_REGISTER",
	"TYPE_SPEC_SIGNED",
	"TYPE_SPEC_STATIC",
	"TYPE_SPEC_UNSIGNED",
	"TYPE_SPEC_VOLATILE",
	"TYPE_SPEC_SHORT"
};

void
type_print(const type_info_t type)
{
	uint32_t i;

	printf("\ttype: %s\n", type_prim_str[type.prim]);

	for (i = 0; i < 9; ++i) {
		printf("\t%s: %s\n", type_spec_str[i], (type.spec & (1<< i) ? "X" : "0"));
	}

}
