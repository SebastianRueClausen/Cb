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
	[AST_LESSER_EQUAL]		= 4,
	[AST_GREATER_EQUAL]		= 4,
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
	
	"AST_IF",
	"AST_WHILE",

	"AST_IDENTIFIER",

	"AST_NOP",

	"_AST_COUNT"
};

const char*
ast_to_str(enum ast_type type)
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
			return false; } }

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

static struct ast_node*
parse_expression(struct lex_file_buffer *fb, struct sym_table *table,
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
	if (next_token.type == TOK_SEMIKOLON || next_token.type == TOK_PAREN_CLOSED)
		return left;

	node_type = op_tok_to_ast(next_token.type);
	if (!node_type) {
		printf("%s\n", lex_tok_str(next_token.type));
		syntax_error(err_loc(fb), "expected expression");
	}

	while (prev_prec < ast_precedence[node_type]) {
		right = parse_expression(fb, table, ast_precedence[node_type]);
		left = make_ast_node(node_type, left, NULL, right);

		if (fb->curr_token.type == TOK_SEMIKOLON ||
			fb->curr_token.type == TOK_PAREN_CLOSED)
			return left;

		node_type = op_tok_to_ast(fb->curr_token.type);
		if (!node_type) 
			syntax_error(err_loc(fb), "expected expression");
	}

	return left;
}

static struct ast_node*
parse_assignment(struct lex_file_buffer *fb, struct sym_table *table,
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

	left = parse_expression(fb, table, 0);
	assert(fb->curr_token.type == TOK_SEMIKOLON);

	left = make_ast_node(type, left, NULL, right);

	return left;	
}

static struct ast_node*
parse_declaration(struct lex_file_buffer *fb, struct sym_table *table)
{
	int id;
	struct lex_token token;
	struct ast_node *right, *left;

	// it should loop through declaration specifiers,
	// and create a bitfield
	if (!type_spec_tok_to_ast(fb->curr_token.type)) {
		printf("%s\n", lex_tok_str(fb->curr_token.type));
		assert(false);
	}

	token = lex_next_token(fb);

	assert(token.type == TOK_IDENTIFIER);

	id = sym_find_entry(table, token.hash);
	if (id == -1)
		id = sym_add_entry(table, token.hash);
	else
		syntax_error(token.err_loc, "redefinition of variabel");

	// inline assignment
	// should also check for assignment list
	if (fb->next_token.type == TOK_ASSIGN) {
		token = lex_next_token(fb);	
		right = make_ast_node(AST_IDENTIFIER, NULL, NULL, NULL);
		left = parse_expression(fb, table, 0);
		left = make_ast_node(AST_ASSIGN, left, NULL, right);

		return left;
	}

	return NULL;
}

static struct ast_node*
parse_compound_statement(struct lex_file_buffer *fb, struct sym_table *table);

static struct ast_node*
parse_else_statement(struct lex_file_buffer *fb, struct sym_table *table)
{
	lex_next_token(fb);
	return parse_compound_statement(fb, table);
}

static struct ast_node*
parse_if_statement(struct lex_file_buffer *fb, struct sym_table *table)
{
	struct ast_node *condition_ast, *true_ast, *false_ast;

	assert(fb->curr_token.type == TOK_KEY_IF);	

	lex_next_token(fb);
	if (!fb->curr_token.type) {
		syntax_error(fb->curr_token.err_loc,
			"expected conditional statement after if");
	}

	condition_ast = parse_expression(fb, table, 0);

	assert(fb->curr_token.type == TOK_PAREN_CLOSED);
	lex_next_token(fb);
	
	true_ast = parse_compound_statement(fb, table);

	if (fb->next_token.type == TOK_KEY_ELSE) {
		lex_next_token(fb);
		false_ast = parse_else_statement(fb, table);	
	}
	else {
		false_ast = NULL;
	}

	// if we get an else after we fill in the NULL		  vvvv
	return make_ast_node(AST_IF, condition_ast, true_ast, false_ast);
}

static struct ast_node*
parse_while_loop(struct lex_file_buffer *fb, struct sym_table *table)
{
	struct ast_node *condition_ast, *body_ast;

	assert(fb->curr_token.type == TOK_KEY_WHILE);

	lex_next_token(fb);
	if (!fb->curr_token.type) {
		syntax_error(fb->curr_token.err_loc,
			"expected conditional statement after while");
	}

	condition_ast = parse_expression(fb, table, 0);

	assert(fb->curr_token.type == TOK_PAREN_CLOSED);
	lex_next_token(fb);

	body_ast = parse_compound_statement(fb, table);

	return make_ast_node(AST_WHILE, condition_ast, NULL, body_ast);	
}

static struct ast_node*
parse_for_loop(struct lex_file_buffer *fb, struct sym_table *table)
{
	assert(fb->curr_token.type == TOK_KEY_FOR);		


}

static struct ast_node*
parse_statement(struct lex_file_buffer *fb, struct sym_table *table)
{
	enum ast_type type;
	struct lex_token token = lex_next_token(fb);	

	switch (token.type) {
		case TOK_KEY_IF:
			return parse_if_statement(fb, table);

		case TOK_KEY_ELSE:
			syntax_error(token.err_loc, "no maching if statement");
			break;

		case TOK_KEY_WHILE:
			return parse_while_loop(fb, table);

		case TOK_BRACE_OPEN:
			return parse_compound_statement(fb, table);

		case TOK_BRACE_CLOSED:
			syntax_error(token.err_loc, "unexpected closed brace");
			break;

		case TOK_IDENTIFIER:
			type = assign_tok_to_ast(fb->next_token.type);
			if (type) {
				lex_next_token(fb);
				return parse_assignment(fb, table, type);
			}
			else {
				syntax_warning(token.err_loc, "result unused");
				return parse_expression(fb, table, 0);
			}

		default:

			// declaration
			type = type_spec_tok_to_ast(token.type);
			if (type) {
				return parse_declaration(fb, table);
			}

			printf("token type : %s\n", lex_tok_str(fb->curr_token.type));
			assert(false);
			break;
	}

	return NULL;
}

static struct ast_node*
parse_compound_statement(struct lex_file_buffer *fb, struct sym_table *table)
{
	struct ast_node *left = NULL, *tree = NULL;
	assert(fb->curr_token.type == TOK_BRACE_OPEN);

	for (;;) {
		tree = parse_statement(fb, table);
		
		if (tree) {
			if (!left)
				left = tree;
			else
				left = make_ast_node(AST_NOP, left, NULL, tree);
		}

		if (fb->curr_token.type == TOK_BRACE_OPEN) {
			lex_next_token(fb);
			parse_compound_statement(fb, table);
		}
		else if (fb->curr_token.type == TOK_BRACE_CLOSED) {
			lex_next_token(fb);
			return left;
		}

	}
}

void
ast_print_tree(struct ast_node *node, int level)
{
	int i;

	++level;
	printf("%s\n", ast_to_str(node->type));
	if (node->left) {
		for (i = 0; i < level; ++i)
			printf("\t");
		ast_print_tree(node->left, level);			
	}
	
	if (node->center) {
		for (i = 0; i < level; ++i)
			printf("\t");
		ast_print_tree(node->center, level);			
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





void
ast_test()
{
	struct lex_file_buffer fb;
	struct ast_node *tree;
	struct sym_table table;

	lex_load_file_buffer(&fb, "../test/test4.c");
	sym_create_table(&table, 10);
	lex_next_token(&fb);

	tree = parse_compound_statement(&fb, &table);
	ast_print_tree(tree, 0);

	ast_cleanup_tree(tree);
	lex_cleanup_file_buffer(&fb);
	sym_cleanup_table(&table);

}

