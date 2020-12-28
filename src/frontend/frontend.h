#ifndef FRONTEND_H
#define FRONTEND_H

#include "../mem.h"
#include "../err.h"

#include <stdbool.h>

typedef struct frontend frontend_t;
typedef struct ast_node ast_node_t;


/* === type === */
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
}
type_spec_t;

typedef enum type_prim
{
	TYPE_PRIM_NONE,
	TYPE_PRIM_VOID,
	TYPE_PRIM_CHAR,
	TYPE_PRIM_INT,
	TYPE_PRIM_FLOAT,
	TYPE_PRIM_DOUBLE,
	_TYPE_PRIM_COUNT
}
type_prim_t;

typedef enum type_compat
{
	TYPE_COMPAT_INCOMPAT,
	TYPE_COMPAT_COMPAT,
	TYPE_COMPAT_PROMOTE_LEFT,
	TYPE_COMPAT_PROMOTE_RIGHT
}
type_compat_t;


typedef struct type_info
{
	enum type_spec				spec;
	enum type_prim				prim;
	uint32_t					indirection;
}
type_info_t;

static const type_info_t NULL_TYPE_INFO =
{
	.prim						= TYPE_PRIM_NONE,
	.spec						= 0,
	.indirection				= 0
};

typedef enum type_literal_type
{
	TYPE_LIT_INT,
	TYPE_LIT_FLOAT,
	TYPE_LIT_STRING
}
type_literal_type_t;

typedef union type_literal_value
{
	double						val_float;
	uint64_t					val_int;

	struct {
								uint32_t size;
								const char* data;
	} str;
}
type_literal_value_t;

void					type_check_validity(type_info_t *type, struct err_location *err_loc);
size_t					type_get_width(const type_info_t type);
type_compat_t			type_compat(type_info_t left, type_info_t right);
type_info_t				type_adapt_to_suffix(type_info_t suffix, type_info_t literal, struct err_location err_loc);
void					type_print(const type_info_t type);
type_info_t				type_deduct_from_literal(type_literal_value_t lit, type_literal_type_t lit_type);
bool					type_compare(type_info_t a, type_info_t b);


/* === sym === */

#include <limits.h>

/* returned if no symbol is found */
#define SYM_ID_GLOBAL_NULL INT32_MIN
#define SYM_ID_LOCAL_NULL INT32_MAX
#define SYM_ID_NULL 0

#define SYM_NULL_HASH 0

typedef int64_t sym_hash_t;
typedef int32_t	sym_id_t;

typedef enum sym_local_kind
{
	SYM_LOCAL_KIND_VARIABLE,
	SYM_LOCAL_KIND_PARAMETER
}
sym_local_kind_t;

typedef enum sym_global_kind
{
	SYM_GLOBAL_KIND_VARIABLE,
	SYM_GLOBAL_KIND_FUNCTION
}
sym_global_kind_t;

typedef struct sym_param
{
	type_info_t				type;
	sym_hash_t				hash;

	/* ugh, we have to store an error loc, since it might be an error later */
	err_location_t			err_loc;
}
sym_param_t;

#define VEC_TYPE sym_param_t
#include "../templates/vec.h"
#undef VEC_TYPE

/* would prob be better to have a list of global, and local
 * entry list, since globals take up more data */
typedef struct sym_local
{
	type_info_t				type;
	sym_local_kind_t		kind;
}
sym_local_t;

typedef struct sym_global
{
	type_info_t				type;
	sym_global_kind_t		kind;

	bool					defined;

	union
	{
		struct
		{
			vec_sym_param_t		params;
			ast_node_t			*body;	
		} function;

		type_literal_value_t	val;
	};
}
sym_global_t;

/* points to a section of the symbol table */
typedef struct sym_scope
{
	uint32_t				size;
}
sym_scope_t;

/* generate vectors */
#define VEC_TYPE sym_local_t
#include "../templates/vec.h"
#undef VEC_TYPE

#define VEC_TYPE sym_global_t
#include "../templates/vec.h"
#undef VEC_TYPE

#define VEC_TYPE sym_scope_t
#include "../templates/vec.h"
#undef VEC_TYPE

#define VEC_TYPE sym_hash_t
#include "../templates/vec.h"
#undef VEC_TYPE

typedef struct sym_table
{
	vec_sym_global_t		globals;
	vec_sym_local_t			locals;

	vec_sym_hash_t			global_hashes;
	vec_sym_hash_t			local_hashes;	

	vec_sym_scope_t			scopes;
}
sym_table_t;

sym_hash_t			sym_hash(const char *key, uint32_t len);

void				sym_create_table(sym_table_t *table, uint32_t count);
void				sym_destroy_table(sym_table_t *table);

sym_id_t			sym_find_global(const sym_table_t *table, sym_hash_t hash);
sym_global_t*		sym_get_global(sym_table_t *table, sym_id_t id);
sym_id_t			sym_declare_global(sym_table_t *table, sym_global_t global, sym_hash_t hash, err_location_t *err_loc);
sym_id_t			sym_define_global(sym_table_t *table, sym_global_t global, sym_hash_t hash, err_location_t *err_loc);


sym_id_t			sym_find_local(const sym_table_t *table, sym_hash_t hash);
sym_local_t*		sym_get_local(sym_table_t *table, sym_id_t id);
sym_id_t			sym_define_local(sym_table_t *table, sym_local_t local, sym_hash_t hash, err_location_t *err_loc);

void*				sym_get_entry(sym_table_t *table, sym_id_t id);
sym_id_t			sym_find_id(const sym_table_t *table, sym_hash_t hash);

void				sym_push_scope(sym_table_t *table);
void				sym_pop_scope(sym_table_t *table);
void				sym_add_params_to_scope(sym_table_t *table, const vec_sym_param_t *params);

void				sym_check_for_anon_params(const vec_sym_param_t *params);
void				sym_check_for_duplicate_params(const vec_sym_param_t *params);




/* === lex === */
typedef enum lex_token_type
{
	/* special */
	TOK_UNKNOWN,				// £

	/* constants */
	TOK_LITERAL,				// 1234 or 1.234
	
	/* operators */
	TOK_PLUS,					// +
	TOK_MINUS,					// -
	TOK_STAR,					// *
	TOK_DIV,					// /
	TOK_MOD,					// %

	TOK_PLUS_ASSIGN,			// +=
	TOK_MINUS_ASSIGN,			// -=
	TOK_STAR_ASSIGN,			// *=
	TOK_DIV_ASSIGN,				// /=
	TOK_MOD_ASSIGN,				// %=

	TOK_INCREASE,				// ++
	TOK_DECREASE,				// --
	TOK_ASSIGN,					// =
	
	/* logical operators */
	TOK_GREATER,				// >
	TOK_LESSER,					// <
	TOK_GREATER_OR_EQUAL,		// >=
	TOK_LESSER_OR_EQUAL,		// <=
	TOK_EQUAL,					// ==
	TOK_NOT_EQUAL,				// !=
	TOK_NOT,					// !
	TOK_AND,					// &&
	TOK_OR,						// ||

	/* bitwise operators */
	TOK_AMPERSAND,				// &
	TOK_NOT_BIT,				// ~
	TOK_OR_BIT,					// |
	TOK_EOR,					// ^
	TOK_LEFT_SHIFT,				// <<
	TOK_RIGHT_SHIFT,			// >>
	
	TOK_AMPERSAND_ASSIGN,		// &=
	TOK_NOT_ASSIGN,				// ~=
	TOK_OR_ASSIGN,				// |=
	TOK_EOR_ASSIGN,				// ^=
	TOK_LEFT_SHIFT_ASSIGN,		// <<=
	TOK_RIGHT_SHIFT_ASSIGN,		// >>=

	/* ctrl operands */
	TOK_PAREN_OPEN,				// (
	TOK_PAREN_CLOSED,			// )
	TOK_BRACKET_OPEN,			// [
	TOK_BRACKET_CLOSED,			// ]
	TOK_BRACE_OPEN,				// {
	TOK_BRACE_CLOSED,			// }
	
	TOK_DOT,					// .
	TOK_ARROW,					// ->
	TOK_SEMIKOLON,				// ;
	TOK_KOLON,					// :
	TOK_COMMA,					// ,
	TOK_ELLIPSIS,				// ...
	TOK_QUERY,					// ?

	/* keywords */
	TOK_KEY_IF,
	TOK_KEY_ELSE,
	TOK_KEY_WHILE,
	TOK_KEY_DO,
	TOK_KEY_FOR,
	TOK_KEY_SWITCH,
	TOK_KEY_CASE,
	TOK_KEY_BREAK,
	TOK_KEY_DEFAULT,
	TOK_KEY_CONTINUE,
	TOK_KEY_RETURN,
	TOK_KEY_GOTO,
	TOK_KEY_INT,
	TOK_KEY_FLOAT,
	TOK_KEY_CHAR,
	TOK_KEY_DOUBLE,
	TOK_KEY_LONG,
	TOK_KEY_SHORT,
	TOK_KEY_VOID,
	TOK_KEY_CONST,
	TOK_KEY_VOLATILE,
	TOK_KEY_REGISTER,
	TOK_KEY_SIGNED,
	TOK_KEY_UNSIGNED,
	TOK_KEY_STRUCT,
	TOK_KEY_ENUM,
	TOK_KEY_UNION,
	TOK_KEY_EXTERN,
	TOK_KEY_STATIC,
	TOK_KEY_SIZEOF,
	TOK_KEY_TYPEDEF,

	TOK_IDENTIFIER,
	TOK_EOF,
	_TOK_COUNT
}
lex_token_type_t;

typedef struct lex_literal
{
	type_info_t					type;
	type_literal_value_t		value;
}
lex_literal_t;

typedef struct lex_token
{
	lex_token_type_t			type;
	err_location_t				err_loc;
	
	union
	{
		lex_literal_t			literal;
		sym_hash_t				hash;
	};
}
lex_token_t;

typedef struct lex_instance
{
	uint32_t					line;
	uint32_t					col;

	char*						filename;
	
	char*						start;
	char*						curr;
	char*						end;

	lex_token_t					last_token;
	lex_token_t					curr_token;
	lex_token_t					next_token;
}
lex_instance_t;

/* @debug: should this be public */
err_location_t		err_loc(const lex_instance_t* fb);
const char*			lex_tok_debug_str(lex_token_type_t type);
lex_instance_t		lex_create_instance(const char* filename);
void				lex_destroy_instance(lex_instance_t* lex_in);
lex_token_t			lex_next_token(frontend_t* f);


/* === ast === */
typedef enum ast_type
{
	AST_UNDEF,

	/* operations */
	AST_ADD,
	AST_MIN,
	AST_MUL,
	AST_DIV,
	AST_MOD,

	/* comparison */
	AST_EQUAL,
	AST_NOT_EQUAL,
	AST_LESSER,
	AST_GREATER,
	AST_LESSER_EQUAL,
	AST_GREATER_EQUAL,

	/* prefix operator */
	AST_UNARY_PLUS,
	AST_UNARY_MINUS,
	AST_UNARY_NOT,

	/* bitwise operaions */
	AST_BIT_NOT,

	/* prefix expression */
	AST_PRE_INCREMENT,
	AST_PRE_DECREMENT,

	/* postfix expression */
	AST_POST_INCREMENT,
	AST_POST_DECREMENT,

	/* pointer stuff (also prefix operaions) */
	AST_DEREF,
	AST_ADDRESS,

	/* @note: should this be part of conversions/casting? */
	AST_PROMOTE,

	/* assignment */
	AST_ASSIGN,

	AST_LITERAL,

	AST_IF,
	AST_WHILE,

	AST_IDENTIFIER,

	AST_NOP,

	_AST_COUNT
}
ast_type_t;

typedef enum ast_value_type
{
	AST_RVALUE,
	AST_LVALUE
}
ast_value_type_t;

typedef struct ast_literal
{
	type_info_t					type;
	type_literal_value_t		value;
}
ast_literal_t;

typedef struct ast_node ast_node_t;

typedef struct ast_node
{
	ast_type_t					type;

	/* @performance: could be index into an array we alloc from */
	ast_node_t					*left;
	ast_node_t					*center;
	ast_node_t					*right;

	ast_value_type_t			value_type;

	union
	{
		ast_literal_t			literal;
		/* used to store the the type of and expression */
		type_info_t				expr_type;
		/* if type is an lvalue */
		sym_id_t				sym_id;
	};
}
ast_node_t;



const char*			ast_to_str(ast_type_t type);
void				ast_print_tree(ast_node_t* node, uint32_t level);
void				ast_print_tree_postorder(ast_node_t *node);
/* @debug: should not be public */
ast_node_t*			parse_statement(frontend_t *f);
ast_node_t*			ast_parse(frontend_t *f);


/* === frontend === */
typedef struct frontend
{
	mem_pool_t					pool;
	
	lex_instance_t				lex;
	sym_table_t					sym_table;
}
frontend_t;

void				frontend_create(frontend_t *f, const char* filename);
void				frontend_destroy(frontend_t *f);


#endif
