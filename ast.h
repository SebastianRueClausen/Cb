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

	AST_INT_LIT,
	AST_CHAR_LIT,
	AST_FLOAT_LIT,

	AST_IF,
	AST_WHILE,

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

void
ast_print_tree(struct ast_node* node, int level);

void
ast_cleanup_tree(struct ast_node* node);

void
ast_test();

#endif
