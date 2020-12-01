#ifndef AST_H
#define AST_H

#include "def.h"
#include "sym.h"
#include "lex.h"

enum ast_type
{
	AST_UNDEF,

	AST_ADD,
	AST_MIN,
	AST_MUL,
	AST_DIV,

	AST_EQUAL,
	AST_NOT_EQUAL,
	AST_LESSER,
	AST_GREATER,
	AST_LESSER_EQUAL,
	AST_GREATER_EQUAL,

	AST_UNARY_PLUS,
	AST_UNARY_MINUS,

	AST_PRE_INCREMENT,
	AST_PRE_DECREMENT,

	AST_DEREF,
	AST_ADDRESS,

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

/* @note: should every literal be a part of the symbol table?? */
struct ast_literal
{
	struct type_info				type;
	union type_literal_value		value;
};

struct ast_node
{
	enum ast_type					type;

	struct ast_node					*left;
	struct ast_node					*center;
	struct ast_node					*right;

	bool							value_type;
	
	union
	{
		struct ast_literal			literal;
		int32_t						sym_id;
	};
};

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

/* temp temp temp temp */
struct ast_node*
parse_statement(struct ast_instance *ast_in);
/* temp temp temp temp */

#endif
