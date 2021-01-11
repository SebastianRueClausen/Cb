#ifndef _F_TYPE_
#define _F_TYPE_

#include "err.h"
#include "f_ast.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum type_primitive
{
    TYPE_NONE,

    /* character types */
    TYPE_SCHAR,
    TYPE_UCHAR,
    TYPE_CHAR,

    /* integer types */
    TYPE_SHORT_SINT,
    TYPE_SHORT_UINT,

    TYPE_SINT,
    TYPE_UINT,

    TYPE_LONG_SINT,
    TYPE_LONG_UINT,

    TYPE_LONG_LONG_SINT,
    TYPE_LONG_LONG_UINT,

    /* floating points */
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_LONG_DOUBLE,

    /* C11 types */
    // TYPE_BOOL,

    /* VOID should properly not be represented a primitive */
    TYPE_VOID,

    _TYPE_PRIMITIVE_COUNT

} type_primitive_t;

typedef enum type_specifier
{
    /* storage specifiers */
    TYPE_EXTERN   = 1 << 0,
    TYPE_STATIC   = 1 << 1,
    TYPE_REGISTER = 1 << 2,

    /* type qualifiers */
    TYPE_CONST    = 1 << 3,
    TYPE_VOLATILE = 1 << 4,

    /* function qualifier */
    TYPE_INLINE = 1 << 5,

} type_specifier_t;

#define _TYPE_SPECIFIER_COUNT 6


typedef struct type
{
    type_primitive_t primitive : 8;
    type_specifier_t specifers : 8;
    uint8_t          indirection;

} type_t;

static const type_t NULL_TYPE = {
    .primitive   = 0,
    .specifers   = 0,
    .indirection = 0,
};

typedef enum type_compatibility
{
    /* if the types are directly compatible */
    TYPE_COMPATIBLE,

    /* if the types are incompatible, but needs conversion/promotion */
    TYPE_CONVSERION,

    /* integer promotion*/
    TYPE_INTEGER_PROMOTION,

    /* if they arent compatible */
    TYPE_INCOMPATIBLE,

    _TYPE_COMPATIBILITY_COUNT

} type_compatibility_t;

typedef struct expr_type_compatibility
{
    /* represents different things for different
	 * expressions, for instance comparison always produce
	 * an SINT, so in that case it represent the
	 * type we compare */
    type_t expr_type;

    type_compatibility_t left_compatibility : 8;
    type_compatibility_t right_compatibility : 8;

} expr_type_compatibility_t;


typedef struct decl_spec decl_spec_t;
typedef struct decl_spec
{
    err_location_t err_loc;

    /* implmented as a linked list for convenience,
	 * however it's properly better to do it as an array */
    decl_spec_t* next;

    enum
    {
        DECL_SPEC_NULL = 0,

        /* to determine primtive type */
        DECL_SPEC_VOID,

        DECL_SPEC_INT,
        DECL_SPEC_CHAR,
        DECL_SPEC_SHORT,
        DECL_SPEC_LONG,
        DECL_SPEC_LONG_LONG,

        DECL_SPEC_SIGNED,
        DECL_SPEC_UNSIGNED,

        DECL_SPEC_FLOAT,
        DECL_SPEC_DOUBLE,

        /* extra specifiers */
        DECL_SPEC_CONST,
        DECL_SPEC_VOLATILE,

        DECL_SPEC_EXTERN,
        DECL_SPEC_REGISTER,
        DECL_SPEC_STATIC,

        DECL_SPEC_INLINE,

        _DECL_SPEC_COUNT

    } spec;

} decl_spec_t;


type_t   f_spec_list_to_type(decl_spec_t* specs);
type_t   f_get_expr_type(type_t left, type_t right, ast_type_t expr_type, err_location_t* err_loc);
uint32_t f_get_type_width(type_t type);
void     f_print_type(type_t type);
void     f_print_expr_type_compatibility(expr_type_compatibility_t compat);
bool     f_is_same_type(type_t a, type_t b);

#endif
