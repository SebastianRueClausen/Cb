#include "ast.h"
#include "lex.h"
#include "def.h"
#include "sym.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

static const int ast_precedence[_AST_COUNT] =
{
	[AST_UNDEF]				= -1,

	[AST_IDENTIFIER]		= 0,
	[AST_INT]				= 0,

	[AST_ADD]				= 1, 
	[AST_MIN]				= 1,
	[AST_MUL]				= 2,
	[AST_DIV]				= 2,

	[AST_EQUAL]				= 3,
	[AST_NOT_EQUAL]			= 3,

	[AST_LESSER]			= 4,
	[AST_GREATER]			= 4,
	[AST_GREATER_EQUAL]		= 4,
	[AST_LESSER_EQUAL]		= 5,
};

static const char* ast_str[] =
{	
	"AST_UNDEF",
	"AST_ADD",
	"AST_MIN",
	"AST_MUL",
	"AST_DIV",

	"AST_EQUAL",
	"AST_NOT_EQUAL",
	"AST_LESSER",
	"AST_GREATER",
	"AST_LESSER_EQUAL",
	"AST_GREATER_EQUAL",

	"AST_ASSIGN",

	"AST_INT",

	"AST_IDENTIFIER",

	"_AST_COUNT"
};

const char*
ast_node_str(enum ast_type type)
{
	return ast_str[type];
}

static enum ast_type
value_tok_to_ast(enum lex_token_type tok)
{
	switch (tok) {
		case TOK_INT_LIT:
			return AST_INT;
		case TOK_IDENTIFIER:
			return AST_IDENTIFIER;
		default:
			return AST_UNDEF;
	}
}

static bool
type_spec_tok_to_ast(enum lex_token_type tok)
{
	switch (tok) {
		case TOK_KEY_INT:	
		case TOK_KEY_SHORT:
		case TOK_KEY_CHAR:
		case TOK_KEY_FLOAT:
		case TOK_KEY_DOUBLE:
			return true;
		default:
			return false;
	}
}

static enum ast_type
op_tok_to_ast(enum lex_token_type tok)
{
	switch (tok) {
		case TOK_PLUS:
			return AST_ADD;
		case TOK_MINUS:
			return AST_MIN;
		case TOK_STAR:
			return AST_MUL;
		case TOK_DIV:
			return AST_DIV;
		case TOK_EQUAL:
			return AST_EQUAL;
		case TOK_NOT_EQUAL:
			return AST_NOT_EQUAL;
		case TOK_GREATER:
			return AST_GREATER;
		case TOK_GREATER_OR_EQUAL:
			return AST_GREATER_EQUAL;
		case TOK_LESSER:
			return AST_LESSER;
		case TOK_LESSER_OR_EQUAL:
			return AST_LESSER_EQUAL;
		default:
			return AST_UNDEF;
	}
}

static enum ast_type
assign_tok_to_ast(enum lex_token_type tok)
{
	switch (tok) {
		case TOK_ASSIGN:
			return AST_ASSIGN;
		default:
			return AST_UNDEF;
	}
}

static struct ast_node*
make_ast_node(enum ast_type type, struct ast_node *left,
			  struct ast_node *center, struct ast_node *right)
{
	struct ast_node* node;

	node					= c_malloc(sizeof(struct ast_node));
	node->left				= left;
	node->center			= center;
	node->right				= right;
	node->type				= type;

	return node;
}

static void
ast_parse_declaration(struct lex_file_buffer *fb, struct sym_table *table)
{
	int id;
	struct lex_token token;

	// it should loop through declaration specifiers,
	// and create a bitfield
	if (!type_spec_tok_to_ast(fb->curr_token.type)) {
		printf("%s\n", lex_tok_str(fb->curr_token.type));
		assert(false);
	}

	token = lex_next_token(fb);

	id = sym_find_entry(table, token.hash);
	if (id == -1) {
		id = sym_add_entry(table, token.hash);
	}

}

struct ast_node*
ast_parse_expression(struct lex_file_buffer *fb, struct sym_table *table,
					 int prev_prec)
{
	struct lex_token token, next_token;
	struct ast_node *left, *right;
	enum ast_type node_type;
	int id;

	// get value token
	token = lex_next_token(fb);
	node_type = value_tok_to_ast(token.type);

	if (!node_type)
		syntax_error(err_loc(fb), "expected expression");

	left = make_ast_node(node_type, NULL, NULL, NULL);

	if (node_type == AST_IDENTIFIER) {
		id = sym_find_entry(table, token.hash);
		if (id == -1)
			syntax_error(err_loc(fb), "use of undeclared identifier");

		left->id = id;
	}
	else {
		left->value = token.value;	
	}

	// get op token
	next_token = lex_next_token(fb);
	if (next_token.type == TOK_SEMIKOLON)
		return left;

	node_type = op_tok_to_ast(next_token.type);
	if (!node_type) {
		printf("%s\n", lex_tok_str(next_token.type));
		syntax_error(err_loc(fb), "expected expression");
	}

	while (prev_prec < ast_precedence[node_type]) {
		right = ast_parse_expression(fb, table, ast_precedence[node_type]);
		left = make_ast_node(node_type, left, NULL, right);

		if (fb->curr_token.type == TOK_SEMIKOLON)
			return left;

		node_type = op_tok_to_ast(fb->curr_token.type);
		if (!node_type) 
			syntax_error(err_loc(fb), "expected expression");
	}

	return left;
}

static struct ast_node*
ast_parse_assignment(struct lex_file_buffer *fb, struct sym_table *table,
					 enum ast_type type)
{
	struct ast_node *left, *right;
	int id;

	if (fb->last_token.type != TOK_IDENTIFIER)
		syntax_error(fb->last_token.err_loc, "assigning to rvalue");

	id = sym_find_entry(table, fb->last_token.hash);
	if (id == -1)
		syntax_error(fb->last_token.err_loc, "use of undeclated identifier");

	right = make_ast_node(AST_IDENTIFIER, NULL, NULL, NULL);

	left = ast_parse_expression(fb, table, 0);

	assert(fb->curr_token.type == TOK_SEMIKOLON);
	lex_next_token(fb);

	left = make_ast_node(type, left, NULL, right);

	return left;	
}

static struct ast_node*
parse_compound_statement(struct lex_file_buffer *fb, struct sym_table *table);

static struct ast_node*
parse_if_statement(struct lex_file_buffer *fb, struct sym_table *table)
{
	struct ast_node *condition_ast, *true_ast, *false_ast = NULL;

	assert(fb->curr_token.type == TOK_KEY_IF);	

	lex_next_token(fb);
	if (!fb->curr_token.type) {
		syntax_error(fb->curr_token.err_loc,
			"expected left paran after if statement");
	}

	condition_ast = ast_parse_expression(fb, table, 0);

	true_ast = parse_compound_statement(fb, table);

	if (fb->curr_token.type == TOK_KEY_ELSE) {
		false_ast = parse_compound_statement(fb, table);
	}

	return make_ast_node(AST_IF, condition_ast, true_ast, false_ast);
}

static struct ast_node*
parse_compound_statement(struct lex_file_buffer *fb, struct sym_table *table)
{
	struct ast_node *tmp = NULL, *left = NULL;
	struct lex_token token;
	enum   ast_type type;

	assert(fb->curr_token.type == TOK_BRACE_OPEN);

	for (;;) {
		token = lex_next_token(fb);	

		// assignment
		type = assign_tok_to_ast(token.type);
		if (type) {
			tmp = ast_parse_assignment(fb, table, type);
			continue;
		}

		// declaration
		type = type_spec_tok_to_ast(token.type);
		if (type) {
			ast_parse_declaration(fb, table);
			continue;
		}

		// keywords and special symbols
		switch (token.type) {
			case TOK_KEY_IF:
				tmp = parse_if_statement(fb, table);
				break;

			case TOK_BRACE_OPEN:
				tmp	= parse_compound_statement(fb, table);
				break;

			case TOK_BRACE_CLOSED:
				// parse closed brace
				lex_next_token(fb);
				return left;

			default:
				printf("default parse compound statement\n");
				assert(false);
		}
	}

	if (tmp) {
		if (left == NULL)
			left = tmp;
		else
			left = make_ast_node(AST_NOP, left, NULL, tmp);
	}
}

void
ast_print_tree(struct ast_node *node, int level)
{
	int i;

	++level;
	printf("%s\n", ast_node_str(node->type));
	if (node->left) {
		for (i = 0; i < level; ++i)
			printf("\t");
		ast_print_tree(node->left, level);			
	}

	if (node->right) {
		for (i = 0; i < level; ++i)
			printf("\t");
		ast_print_tree(node->right, level);
	}
}

void
ast_cleanup_tree(struct ast_node *node)
{
	if (node->left)
		ast_cleanup_tree(node->left);

	if (node->center)
		ast_cleanup_tree(node->center);

	if (node->right)
		ast_cleanup_tree(node->right);

	c_free(node);
}

