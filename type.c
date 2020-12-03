#include "type.h"

#include <stdio.h>

/* check for conflicts of a type info */
void
type_check_conflicts(struct type_info type, struct err_location *err_loc)
{
	switch (type.prim) {
		case TYPE_PRIM_FLOAT:
			if (type.spec & TYPE_SPEC_SIGNED || type.spec & TYPE_SPEC_UNSIGNED) {
				syntax_error(*err_loc, "type float can not be specified "
										"as signed or unsigned");
			} else if (type.spec & TYPE_SPEC_LONG) {
				syntax_error(*err_loc, "type float can not be specified as long");
			} else if (type.spec & TYPE_SPEC_SHORT) {
				syntax_error(*err_loc, "type float can not be specified as short");
			}
			break;

		case TYPE_PRIM_DOUBLE:	
			if (type.spec& TYPE_SPEC_SIGNED || type.spec& TYPE_SPEC_UNSIGNED) {
				syntax_error(*err_loc, "type double can not be specified "
										"as signed or unsigned");
			} else if (type.spec & TYPE_SPEC_LONG) {
				syntax_error(*err_loc, "type double can not be specified as long");
			} else if (type.spec & TYPE_SPEC_SHORT) {
				syntax_error(*err_loc, "type double can not be specified as short");
			}
			break;

		default:
			if (type.spec & TYPE_SPEC_SIGNED && type.spec & TYPE_SPEC_UNSIGNED) {
				syntax_error(*err_loc, "type can not be specified as signed and unsigned");
			} else if (type.spec & TYPE_SPEC_LONG && type.spec & TYPE_SPEC_SHORT) {
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
type_get_width(const struct type_info type)
{
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

/* check the compatebillity of two types */
enum type_compat
type_compat(struct type_info left, struct type_info right)
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

	/* @TODO we should check how much we should widen with */
	if (left_width > right_width) {
		return TYPE_COMPAT_WIDEN_RIGHT;
	}

	if (left_width < right_width) {
		return TYPE_COMPAT_WIDEN_LEFT;
	}

	return TYPE_COMPAT_COMPAT;
}

struct type_info
type_deduct_from_literal(union type_literal_value lit,
						 enum type_literal_type lit_type)
{
	struct type_info type = NULL_TYPE_INFO;

	type.indirection = 0;
	type.spec = 0;

	if (lit_type == TYPE_LIT_INT) {
		/* we make it the smallest possible literal */
		if (lit.val_int <= TYPE_UCHAR_MAX) {
			type.prim =	TYPE_PRIM_CHAR;
		} else if (lit.val_int <= TYPE_USHORT_MAX) {
			type.prim = TYPE_PRIM_INT;
			type.spec &= TYPE_SPEC_SHORT;
		} else if (lit.val_int <= TYPE_UINT_MAX) {
			type.prim = TYPE_PRIM_INT;
		} else {
			type.prim = TYPE_PRIM_INT;
			type.spec &= TYPE_SPEC_LONG;
		}

		/* it should always be signed, unless there is a "u" suffix */
		type.spec &= TYPE_SPEC_SIGNED;
		
	} else if (lit_type == TYPE_LIT_FLOAT) {
	
		/* @note: how do we decide if it is a double or float */
		type.prim = TYPE_PRIM_DOUBLE;
	} else { /* string */
		type.prim = TYPE_PRIM_CHAR;
		/* should this be const?? */
		type.spec = TYPE_SPEC_CONST;
		type.indirection = 1;
	}

	return type;
}

/* should already be checked that the suffix and literal are compat, 
 * we just change the type of the literal so it matches the suffix */
struct type_info
type_combine_suffix_and_lit(struct type_info suffix, struct type_info literal)
{
	if (suffix.spec == TYPE_SPEC_UNSIGNED) {
		literal.spec &= TYPE_SPEC_UNSIGNED;	
	}

	return literal;
}

static const char* type_prim_str[] =
{
	"TYPE_PRIM_NONE",
	"TYPE_PRIM_INT",
	"TYPE_PRIM_FLOAT",
	"TYPE_PRIM_CHAR",
	"TYPE_PRIM_DOUBLE"
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
type_print(const struct type_info type)
{
	uint32_t i;

	printf("\ttype: %s\n", type_prim_str[type.prim]);

	for (i = 0; i < 9; ++i) {
		printf("\t%s: %s\n", type_spec_str[i], (type.spec & (1<< i) ? "X" : "0"));
	}

}
