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

	AST_ASSIGN,

	AST_INT,

	AST_IF,

	AST_IDENTIFIER,

	AST_NOP,

	_AST_COUNT
};

struct ast_node
{
	enum						ast_type type;
	struct ast_node				*left;
	struct ast_node				*center;			// mainly used for condit statements
	struct ast_node				*right;
	
	union
	{
		union const_value		value;
		int						id;
	};
};

struct ast_node*
ast_parse_expression(struct lex_file_buffer* fb, struct sym_table* table, int prev_prec);

void
ast_print_tree(struct ast_node* node, int level);

void
ast_cleanup_tree(struct ast_node* node);

#endif
