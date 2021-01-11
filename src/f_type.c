#include "f_type.h"
#include "f_ast.h"

#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

static const char* type_primitive_str[_TYPE_PRIMITIVE_COUNT] = {
    "TYPE_NONE",       "TYPE_SCHAR",      "TYPE_UCHAR",          "TYPE_CHAR",
    "TYPE_SHORT_SINT", "TYPE_SHORT_UINT", "TYPE_SINT",           "TYPE_UINT",
    "TYPE_LONG_SINT",  "TYPE_LONG_UINT",  "TYPE_LONG_LONG_SINT", "TYPE_LONG_LONG_UINT",
    "TYPE_FLOAT",      "TYPE_DOUBLE",     "TYPE_LONG_DOUBLE",    "TYPE_VOID",
};

static const char* type_specifier_str[_TYPE_SPECIFIER_COUNT] = {
    "TYPE_EXTERN", "TYPE_STATIC", "TYPE_REGISTER", "TYPE_CONST", "TYPE_VOLATILE", "TYPE_INLINE",
};

static const char* type_compatibility_str[_TYPE_COMPATIBILITY_COUNT] = {
    "TYPE_COMPATIBLE",
    "TYPE_CONVSERION",
    "TYPE_INTEGER_PROMOTION",
    "TYPE_INCOMPATIBLE",
};

typedef enum scalar_type
{
    SCALAR_TYPE_SIGNED,
    SCALAR_TYPE_UNSIGNED,
    SCALAR_TYPE_FLOATING,
    SCALAR_TYPE_PTR

} scalar_type_t;

typedef struct primitive_info
{
    uint8_t width;

    /* for integer promotion */
    uint8_t rank;

    scalar_type_t scalar_type : 8;
} primitive_info_t;

static const primitive_info_t prim_info[_TYPE_PRIMITIVE_COUNT] = {
    [TYPE_SCHAR] = { 8, 1, SCALAR_TYPE_SIGNED },
    [TYPE_UCHAR] = { 8, 1, SCALAR_TYPE_UNSIGNED },
    [TYPE_CHAR]  = { 8, 1, SCALAR_TYPE_SIGNED },

    [TYPE_SHORT_SINT] = { 16, 2, SCALAR_TYPE_SIGNED },
    [TYPE_SHORT_UINT] = { 16, 2, SCALAR_TYPE_UNSIGNED },

    [TYPE_SINT] = { 32, 3, SCALAR_TYPE_SIGNED },
    [TYPE_UINT] = { 32, 3, SCALAR_TYPE_UNSIGNED },

    [TYPE_LONG_SINT] = { 64, 4, SCALAR_TYPE_SIGNED },
    [TYPE_LONG_UINT] = { 64, 4, SCALAR_TYPE_UNSIGNED },

    [TYPE_LONG_LONG_SINT] = { 64, 5, SCALAR_TYPE_SIGNED },
    [TYPE_LONG_LONG_UINT] = { 64, 5, SCALAR_TYPE_UNSIGNED },

    /* integer promotion rank not used for floating point */
    [TYPE_FLOAT]       = { 32, 0, SCALAR_TYPE_FLOATING },
    [TYPE_DOUBLE]      = { 64, 0, SCALAR_TYPE_FLOATING },
    [TYPE_LONG_DOUBLE] = { 64, 0, SCALAR_TYPE_FLOATING },

    [TYPE_VOID] = { 0, 0, 0 }
};

uint32_t
f_get_type_width(type_t type)
{
    if (type.indirection)
        return 64;

    return prim_info[type.primitive].width;
}

static inline bool
is_small_int(type_t type)
{
    const uint32_t rank = prim_info[type.primitive].rank;

    if (rank != 0 && rank < 3 && !type.indirection)
        return true;
    else
        return false;
}

static inline bool
is_int(type_t type)
{
    const scalar_type_t info = prim_info[type.primitive].scalar_type;

    if ((info == SCALAR_TYPE_UNSIGNED || info == SCALAR_TYPE_SIGNED) && !type.indirection)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static inline bool
is_ptr(type_t type)
{
    return type.indirection;
}

/* if both types are ints */
static type_t
int_int_expr_compatebility(type_t left, type_t right)
{
    const primitive_info_t left_info  = prim_info[left.primitive];
    const primitive_info_t right_info = prim_info[right.primitive];

    type_t expr_type = NULL_TYPE;

    if (left_info.rank < right_info.rank)
    {
        expr_type = right;
    }
    else if (right_info.rank < left_info.rank)
    {
        expr_type = left;
    }
    else
    {
        /* if they are same rank, we further check that they are one
		 * is unsigned, in which case that take precedence */
        if (left_info.scalar_type == SCALAR_TYPE_UNSIGNED)
        {
            expr_type = left;
        }
        else
        {
            expr_type = right;
        }
    }

    /* we just have to check that they aren't small ints */
    if (is_small_int(expr_type))
    {
        /* we always promote small ints to sint */
        expr_type.primitive = TYPE_SINT;
    }

    return expr_type;
}

/* if both operands are pointers */
static type_t
ptr_ptr_expr_compatibility(type_t left, type_t right, ast_type_t operation, err_location_t* err_loc)
{
    switch (operation)
    {
    case AST_ASSIGN:

        if (left.primitive != right.primitive &&
            (left.primitive != TYPE_VOID || right.primitive != TYPE_VOID))
        {
            syntax_warning(*err_loc, "incompatible pointer types");
        }

        return left;

    case AST_EQUAL:
    case AST_NOT_EQUAL:
    case AST_GREATER_EQUAL:
    case AST_LESSER_EQUAL:
    case AST_GREATER:
    case AST_LESSER:
        return left;

    default:
        syntax_error(*err_loc, "invalid types for binary operator");
        assert(false);
        break;
    }
}

/* one is an int, one is a pointer */
static type_t
ptr_int_expr_compatibility(type_t left, type_t right, ast_type_t operation, err_location_t* err_loc)
{
    switch (operation)
    {
    /* @todo: add for the other types of assignment */
    case AST_ASSIGN:
		
        if (!is_ptr(left))
        {
            syntax_warning(*err_loc, "ptr to int conversion");
            return right;
        }
        else
        {
            syntax_warning(*err_loc, "int to ptr conversion");
            return left;
        }

    case AST_EQUAL:
    case AST_NOT_EQUAL:
    case AST_GREATER_EQUAL:
    case AST_LESSER_EQUAL:
    case AST_GREATER:
    case AST_LESSER:

        syntax_warning(*err_loc, "comparison between pointer and int");

        if (!is_ptr(left))
        {
            return right;
        }
        else
        {
            return left;
        }

    /* pointer arithmetics */
    case AST_ADD:
    case AST_MIN:

        if (!is_ptr(left))
        {
            return right;
        }
        else
        {
            return left;
        }

	/* @todo: this should be in another function */
    case AST_PRE_INCREMENT:
    case AST_POST_INCREMENT:
    case AST_PRE_DECREMENT:
    case AST_POST_DECREMENT:

		return left;		

    default:
        syntax_error(*err_loc, "ptr not allowed for this binary operation");
        assert(false);
    }
}

bool
f_is_same_type(type_t a, type_t b)
{
    if (a.primitive == b.primitive && a.indirection == b.indirection)
    {
        return true;
    }
    else
    {
        return false;
    }
}

type_t
f_get_expr_type(type_t left, type_t right, ast_type_t expr_type, err_location_t* err_loc)
{
    /* void is of course never compatible */
    if ((left.primitive == TYPE_VOID && !is_ptr(left)) ||
        (right.primitive == TYPE_VOID && !is_ptr(right)))
    {
        syntax_error(*err_loc, "type void is not compatible in expression");
    }

    if (is_int(left) && is_int(right))
    {
        return int_int_expr_compatebility(left, right);
    }

    if (is_ptr(left) && is_ptr(right))
    {
        return ptr_ptr_expr_compatibility(left, right, expr_type, err_loc);
    }

    /* if only one side is a ptr */
    if (is_ptr(left) + is_ptr(right) == 1)
    {
        return ptr_int_expr_compatibility(left, right, expr_type, err_loc);
    }

    if (f_is_same_type(left, right))
    {
        return left;
    }

    syntax_error(*err_loc, "types are not compatible!!");
    assert(false);
}

inline static bool
is_decl_spec_type_specifier(uint32_t spec)
{
    switch (spec)
    {
    case DECL_SPEC_INT:
    case DECL_SPEC_CHAR:
    case DECL_SPEC_VOID:
    case DECL_SPEC_FLOAT:
    case DECL_SPEC_DOUBLE:
    case DECL_SPEC_LONG:
        return true;

    default:
        return false;
    }
}

#define BIT(x) (1 << x)

static type_primitive_t
determine_primitive(uint64_t bit_field, err_location_t* err_loc)
{
    type_primitive_t primitive;

    /* determine the primitve type */
    if (bit_field & BIT(DECL_SPEC_CHAR))
    {
        if (bit_field & BIT(DECL_SPEC_UNSIGNED))
        {
            primitive = TYPE_UCHAR;
        }
        else
        {
            primitive = TYPE_SCHAR;
        }

        if (bit_field & (BIT(DECL_SPEC_SHORT) | BIT(DECL_SPEC_LONG) | BIT(DECL_SPEC_LONG_LONG)))
        {
            syntax_error(*err_loc, "char is not allowed to be defined as short, long or long long");
        }
    }
    else if (bit_field & BIT(DECL_SPEC_FLOAT))
    {
        primitive = TYPE_FLOAT;

        if (bit_field & (BIT(DECL_SPEC_SHORT) | BIT(DECL_SPEC_LONG) | BIT(DECL_SPEC_LONG_LONG) |
                         BIT(DECL_SPEC_SIGNED) | BIT(DECL_SPEC_UNSIGNED)))
        {
            syntax_error(*err_loc,
                         "float may not be defined as short, long, long long, signed or unsigned");
        }
    }
    else if (bit_field & BIT(DECL_SPEC_DOUBLE))
    {
        if (bit_field & BIT(DECL_SPEC_LONG))
        {
            primitive = TYPE_LONG_DOUBLE;
        }
        else
        {
            primitive = TYPE_DOUBLE;
        }

        if (bit_field & (BIT(DECL_SPEC_SHORT) | BIT(DECL_SPEC_LONG_LONG) | BIT(DECL_SPEC_SIGNED) |
                         BIT(DECL_SPEC_UNSIGNED)))
        {
            syntax_error(*err_loc,
                         "double may not be defined as short, long long, signed or unsigned");
        }
    }
    else if (bit_field & BIT(DECL_SPEC_VOID))
    {
        primitive = TYPE_VOID;

        if (bit_field & (BIT(DECL_SPEC_SHORT) | BIT(DECL_SPEC_LONG_LONG) | BIT(DECL_SPEC_SIGNED) |
                         BIT(DECL_SPEC_UNSIGNED)))
        {
            syntax_error(*err_loc,
                         "void may not be defined as short, long, long long, signed or unsigned");
        }
    }
    /* when there isnt any specific type defined, we have to deduct the type
	 * since 'unsigned i' is legal */
    else
    {
        if (bit_field & BIT(DECL_SPEC_SHORT))
        {
            if (bit_field & BIT(DECL_SPEC_UNSIGNED))
            {
                primitive = TYPE_SHORT_UINT;
            }
            else
            {
                primitive = TYPE_SHORT_SINT;
            }
        }
        else if (bit_field & BIT(DECL_SPEC_LONG))
        {
            if (bit_field & BIT(DECL_SPEC_UNSIGNED))
            {
                primitive = TYPE_LONG_UINT;
            }
            else
            {
                primitive = TYPE_LONG_SINT;
            }

            /* we shoulent have to check for long long*/
        }
        else if (bit_field & BIT(DECL_SPEC_LONG_LONG))
        {
            if (bit_field & BIT(DECL_SPEC_UNSIGNED))
            {
                primitive = TYPE_LONG_LONG_UINT;
            }
            else
            {
                primitive = TYPE_LONG_LONG_SINT;
            }
        }
        else if (bit_field & BIT(DECL_SPEC_INT))
        {
            if (bit_field & BIT(DECL_SPEC_UNSIGNED))
            {
                primitive = TYPE_UINT;
            }
            else
            {
                primitive = TYPE_SINT;
            }
        }
        else if (bit_field & BIT(DECL_SPEC_UNSIGNED))
        {
            primitive = TYPE_UINT;
        }
        else if (bit_field & BIT(DECL_SPEC_SIGNED))
        {
            primitive = TYPE_SINT;
        }
        else
        {
            primitive = TYPE_SINT;
            syntax_warning(*err_loc, "missing type specifier, defaulting to int");
        }
    }

    return primitive;
}

type_t
f_spec_list_to_type(decl_spec_t* specs)
{
    uint64_t    bit_field = 0;
    decl_spec_t current;
    type_t      type = NULL_TYPE;

    while (specs)
    {
        current = *specs;

        /* check for duplicate */
        if (bit_field & BIT(current.spec))
        {
            /* non-type specifier only isue a warning when duplicate */
            if (!is_decl_spec_type_specifier(current.spec))
            {
                syntax_warning(current.err_loc, "duplicate specifier");
                continue;
            }
            else
            {
                /* check for long long */
                if ((current.spec == DECL_SPEC_LONG) && (bit_field & BIT(DECL_SPEC_LONG)))
                {
                    bit_field |= BIT(DECL_SPEC_LONG_LONG);
                    bit_field &= ~BIT(DECL_SPEC_LONG);
                    current.spec = DECL_SPEC_LONG_LONG;
                }
                else
                {
                    syntax_error(current.err_loc, "duplicate type specifier");
                }
            }
        }
        else
        {
            bit_field |= BIT(current.spec);
        }

        switch (current.spec)
        {
        case DECL_SPEC_EXTERN:
            type.specifers |= TYPE_EXTERN;
            continue;
        case DECL_SPEC_STATIC:
            type.specifers |= TYPE_STATIC;
            continue;
        case DECL_SPEC_REGISTER:
            type.specifers |= TYPE_REGISTER;
            continue;
        case DECL_SPEC_CONST:
            type.specifers |= TYPE_CONST;
            continue;
        case DECL_SPEC_VOLATILE:
            type.specifers |= TYPE_VOLATILE;
            continue;
        case DECL_SPEC_INLINE:
            type.specifers |= TYPE_INLINE;
            continue;

        case DECL_SPEC_UNSIGNED:
            if (bit_field & BIT(DECL_SPEC_SIGNED))
            {
                syntax_error(current.err_loc, "type may not be defined as signed and unsigned");
            }
            continue;

        case DECL_SPEC_SIGNED:
            if (bit_field & BIT(DECL_SPEC_UNSIGNED))
            {
                syntax_error(current.err_loc, "type may not be defined as signed and unsigned");
            }
            continue;

        case DECL_SPEC_SHORT:
            if (bit_field & (BIT(DECL_SPEC_LONG) | BIT(DECL_SPEC_LONG_LONG)))
            {
                syntax_error(current.err_loc,
                             "type may not be defined as short and long or long long");
            }
            continue;

        case DECL_SPEC_LONG:
            if (bit_field & BIT(DECL_SPEC_SHORT))
            {
                syntax_error(current.err_loc, "type may not be defined as long and short");
            }
            continue;

        case DECL_SPEC_LONG_LONG:
            if (bit_field & (BIT(DECL_SPEC_SHORT) | BIT(DECL_SPEC_LONG)))
            {
                syntax_error(current.err_loc,
                             "type may not be defined as long long and long or short");
            }
            continue;

        default:
            continue;
        }
    }

    /* @note: we just report the last qualifier as error if we can't determine a
	 * primitive */
    type.primitive = determine_primitive(bit_field, &current.err_loc);

    return type;
}

void
f_print_type(type_t type)
{
    uint32_t i;

    printf("Primitive type : %s\n", type_primitive_str[type.primitive]);
    printf("Indirection    : %u\n", type.indirection);

    for (i = 0; i < _TYPE_SPECIFIER_COUNT; ++i)
    {
        if (type.specifers & (1 << i))
            printf("\t%s : X\n", type_specifier_str[i]);
        else
            printf("\t%s : 0\n", type_specifier_str[i]);
    }
}

void
f_print_expr_type_compatibility(expr_type_compatibility_t compat)
{
    printf("left compatibility : %s\n", type_compatibility_str[compat.left_compatibility]);
    printf("right compatibility : %s\n", type_compatibility_str[compat.right_compatibility]);

    printf("Expression type : %s\n", type_primitive_str[compat.expr_type.primitive]);
}
