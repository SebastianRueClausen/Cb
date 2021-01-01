#ifndef _F_AST_
#define _F_AST_

#include "f_def.h"
#include "type.h"
#include "symbol.h"


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

	/* conditional */
	AST_IF,
	AST_WHILE,

	/* primary */
	AST_LITERAL,
	AST_IDENTIFIER,
	AST_FUNCTION_CALL,

	AST_NOP,

	_AST_COUNT

} ast_type_t;


typedef enum ast_value_type
{
	AST_RVALUE,
	AST_LVALUE

} ast_value_type_t;


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
		literal_t				literal;
		/* used to store the the type of and expression */
		type_info_t				expr_type;
		/* if type is an lvalue */
		sym_id_t				sym_id;
	};

} ast_node_t;

typedef struct parser parser_t;

const char*			f_ast_type_to_str(ast_type_t type);
void				f_print_ast(ast_node_t* node, uint32_t level);
void				f_print_ast_postorder(ast_node_t *node);

ast_node_t*			f_make_ast_node(parser_t *parser, ast_type_t type,
									ast_node_t *left, ast_node_t *center,
									ast_node_t *right);

#endif
