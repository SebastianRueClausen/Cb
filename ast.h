#ifndef AST_H
#define AST_H

#include "def.h"
#include "sym.h"
#include "lex.h"

enum ast_type
{
	AST_UNDEF, AST_ADD, AST_MIN, AST_MUL, AST_DIV, AST_INT, _AST_COUNT
};

static const char*
ast_type_to_str[] = {	
	"AST_UNDEF", "AST_ADD", "AST_MIN", "AST_MUL", "AST_DIV", "AST_INT"
};

struct ast_node
{
	enum				ast_type type;
	union const_value	value;
	struct				ast_node* left;
	struct				ast_node* right;
};

struct ast_node*
ast_parse_expression(struct lex_file_buffer* fb, struct sym_table* table, int prev_prec);

void
ast_print_tree(struct ast_node* node, int level);

void
ast_cleanup_tree(struct ast_node* node);

#endif
