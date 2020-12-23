#ifndef AST_H
#define AST_H

#include "def.h"
#include "sym.h"
#include "lex.h"

/* @refactor: perhaps combine lex, sym, and ast more into a single frontend
 * module, to likely increase performance, simplicity and sepererate better 
 * from backend, should also split the code more naturally into smaller 
 * source files */

enum ast_type
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
};

enum ast_value_type
{
	AST_RVALUE,
	AST_LVALUE
};

struct ast_literal
{
	struct type_info				type;
	union type_literal_value		value;
};

struct ast_node
{
	enum ast_type					type;

	/* @performance: could be index into an array we alloc from */
	struct ast_node					*left;
	struct ast_node					*center;
	struct ast_node					*right;

	bool							value_type;

	union
	{
		struct ast_literal			literal;
		/* used to store the the type of and expression */
		struct type_info			expr_type;
		/* if type is an lvalue */
		int32_t						sym_id;
	};
};

/* @performance: could be more efficient to just have a single structure,
 * to hold all data for the current translation unit, to avoid pointer fetching */
struct ast_instance
{
	struct mem_pool					pool;
	struct ast_node					*tree;

	struct lex_instance				*lex_in;
	struct sym_table				*sym_table;
};

const char*
ast_to_str(enum ast_type type);

void
ast_print_tree(struct ast_node* node, uint32_t level);

void
ast_print_tree_postorder(struct ast_node *node);

void
ast_test();

struct ast_instance
ast_create_instance(struct lex_instance *lex_in, struct sym_table *table);

void
ast_destroy_instance(struct ast_instance *ast_in);

/* @debug: should not be public */
struct ast_node*
parse_statement(struct ast_instance *ast_in);

#endif
