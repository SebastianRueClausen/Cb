#include "ast.h"
#include "lex.h"
#include "def.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

static const int ast_precedence[_AST_COUNT] =
{
	[AST_INT] = 0,

	[AST_ADD]		= 10, 
	[AST_MIN]		= 10,
	[AST_MUL]		= 20,
	[AST_DIV]		= 20,
};

static enum ast_type
value_node_type(enum lex_token_type tok)
{
	switch (tok) {
		case TOK_CONSTANT_INT:
			return AST_INT;
		default:
			return AST_UNDEF;
	}
}

static enum ast_type
operation_node_type(enum lex_token_type tok)
{
	switch (tok) {
		case TOK_OP_PLUS:
			return AST_ADD;
		case TOK_OP_MINUS:
			return AST_MIN;
		case TOK_OP_STAR:
			return AST_MUL;
		case TOK_OP_DIV:
			return AST_DIV;
		default:
			return AST_UNDEF;
	}
}

static struct ast_node*
make_ast_node(enum ast_type type, struct ast_node* left,
				struct ast_node* right)
{
	struct ast_node* node;

	node = (struct ast_node*)c_malloc(sizeof(struct ast_node));
	node->left = left;
	node->right = right;
	node->type = type;

	return node;
}

struct ast_node*
parse_expression(struct lex_file_buffer* fb, int prev_prec)
{
	struct lex_token token, next_token;
	struct ast_node *left, *right;
	enum ast_type node_type;

	// get value token
	lex_next_token(fb, &token);
	node_type = value_node_type(token.type);
	if (node_type) {
		left = make_ast_node(node_type, NULL, NULL);
		left->int_value = token.value_int;
	}
	else {
		assert(false);
	}

	// get op token
	lex_next_token(fb, &next_token);
	node_type = operation_node_type(next_token.type);
	if (!node_type) {
		return left;
	}

	// while the precedense is larger than last time aroung
	while (prev_prec < ast_precedence[node_type]) {
		right = parse_expression(fb, ast_precedence[node_type]);

		return make_ast_node(node_type, left, right);
	}

	return left;
}

