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
	[AST_INT_LIT]			= 0,
	[AST_CHAR_LIT]			= 0,
	[AST_FLOAT_LIT]			= 0,

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

static const char* ast_str[_AST_COUNT] =
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

	"AST_INT_LIT",
	"AST_CHAR_LIT",
	"AST_FLOAT_LIT",
	
	"AST_IF",
	"AST_WHILE",

	"AST_IDENTIFIER",

	"AST_NOP"
};

const char*
ast_to_str(enum ast_type type)
{
	return ast_str[type];
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

static bool
is_rvalue(enum lex_token_type tok)
{
	switch (tok) {
		case TOK_INT_LIT:
		case TOK_FLOAT_LIT:
			return true;
		default:
			return false;
	}
}

static struct ast_node*
make_ast_node(enum ast_type type, struct ast_node *left,
			  struct ast_node *center, struct ast_node *right)
{
	struct ast_node *node;

	node					= c_malloc(sizeof(struct ast_node));
	node->left				= left;
	node->center			= center;
	node->right				= right;
	node->type				= type;

	return node;
}

static struct ast_node*
make_rvalue_node(const struct lex_token *token)
{
	struct ast_node *node;

	switch (token->type) {
		case TOK_INT_LIT:
			if (token->value.val_int >= 0 && token->value.val_int < 255) {
				node = make_ast_node(AST_CHAR_LIT, NULL, NULL, NULL);
			}
			else {
				node = make_ast_node(AST_INT_LIT, NULL, NULL, NULL);
			}
			break;

		case TOK_FLOAT_LIT:
			node = make_ast_node(AST_FLOAT_LIT, NULL, NULL, NULL);

		default:
			assert(false);
	}

	node->value = token->value;

	return node;
}

static struct ast_node*
make_lvalue_node(const struct lex_token *token, const struct sym_table *table)
{
	int id;
	struct ast_node *node;

	id = sym_find_entry(table, token->hash);

	if (id == -1)
		syntax_error(token->err_loc, "use of undeclared identifier");

	node = make_ast_node(AST_IDENTIFIER, NULL, NULL, NULL);
	node->id = id;
	
	return node;
}

static struct ast_node*
parse_expression(struct lex_file_buffer *fb, struct sym_table *table, int prev_prec)
{
	struct lex_token token, next_token;
	struct ast_node *left = NULL, *right;
	enum ast_type node_type;

	// get value token
	token = lex_next_token(fb);

	if (token.type == TOK_IDENTIFIER) {
		left = make_lvalue_node(&token, table);
	}
	else if (is_rvalue(token.type)) {
		left = make_rvalue_node(&token);	
	}
	else {
		syntax_error(token.err_loc, "expected expression");
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
parse_compound_statement(struct lex_file_buffer *fb, struct sym_table *table);

static struct ast_node*
parse_statement(struct lex_file_buffer *fb, struct sym_table *table);


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

inline static void
set_sym_type(struct sym_entry *entry, enum sym_type type, const struct err_location *err_loc)
{
	if (entry->type)
		syntax_error(*err_loc, "can not combine with prev specifier");			
	else if (entry->type == type)
		syntax_error(*err_loc, "duplicate specifier");
	else
		entry->type = type;

	
}

inline static void
set_sym_spec(struct sym_entry *entry, enum sym_spec spec, const struct err_location *err_loc)
{
	if (entry->specifiers & spec)
		syntax_error(*err_loc, "duplicate specifier");
	else
		entry->specifiers |= spec;
}

static struct sym_entry
parse_sym_type(struct lex_file_buffer *fb)
{
	struct sym_entry sym_entry;

	sym_entry.specifiers = 0;
	sym_entry.type = SYM_NONE;
	
	for (;;) {
		switch (fb->curr_token.type) {
			case TOK_KEY_INT:
				set_sym_type(&sym_entry, SYM_INT, &fb->curr_token.err_loc);
				break;
			case TOK_KEY_FLOAT:
				set_sym_type(&sym_entry, SYM_FLOAT, &fb->curr_token.err_loc);	
				break;
			case TOK_KEY_CHAR:
				set_sym_type(&sym_entry, SYM_CHAR, &fb->curr_token.err_loc);	
				break;
			case TOK_KEY_DOUBLE:
				set_sym_type(&sym_entry, SYM_DOUBLE, &fb->curr_token.err_loc);
				break;
			case TOK_KEY_CONST:
				set_sym_spec(&sym_entry, SYM_CONST, &fb->curr_token.err_loc);
				break;
			case TOK_KEY_STATIC:
				set_sym_spec(&sym_entry, SYM_CONST, &fb->curr_token.err_loc);
				break;
			case TOK_KEY_UNSIGNED:
				set_sym_spec(&sym_entry, SYM_UNSIGNED, &fb->curr_token.err_loc);
				break;
			case TOK_KEY_SIGNED:
				set_sym_spec(&sym_entry, SYM_SIGNED, &fb->curr_token.err_loc);
				break;
			case TOK_KEY_SHORT:
				set_sym_spec(&sym_entry, SYM_SHORT, &fb->curr_token.err_loc);	
				break;
			case TOK_KEY_LONG:
				set_sym_spec(&sym_entry, SYM_LONG, &fb->curr_token.err_loc);
				break;
			case TOK_KEY_REGISTER:
				set_sym_spec(&sym_entry, SYM_REGISTER, &fb->curr_token.err_loc);
				break;
			case TOK_KEY_VOLATILE:
				set_sym_spec(&sym_entry, SYM_VOLATILE, &fb->curr_token.err_loc);
				break;
			case TOK_KEY_EXTERN:
				set_sym_spec(&sym_entry, SYM_EXTERN, &fb->curr_token.err_loc);
				break;
			default:
				return sym_entry;
		}

		lex_next_token(fb);
	}
}

static void
check_sym_conflicts(struct sym_entry *entry, struct err_location *err_loc)
{
	switch (entry->type) {
		case SYM_FLOAT:
			if (entry->specifiers & SYM_SIGNED || entry->specifiers & SYM_UNSIGNED)
				syntax_error(*err_loc, "type float can not be specified as signed or unsigned");
			if (entry->specifiers & SYM_LONG)
				syntax_error(*err_loc, "type float can not be specified as long");
			if (entry->specifiers & SYM_SHORT)
				syntax_error(*err_loc, "type float can not be specified as short");

		case SYM_DOUBLE:	
			if (entry->specifiers & SYM_SIGNED || entry->specifiers & SYM_UNSIGNED)
				syntax_error(*err_loc, "type double can not be specified as signed or unsigned");
			if (entry->specifiers & SYM_LONG)
				syntax_error(*err_loc, "type double can not be specified as long");
			if (entry->specifiers & SYM_SHORT)
				syntax_error(*err_loc, "type double can not be specified as short");

		default:
			if (entry->specifiers & SYM_SIGNED && entry->specifiers & SYM_UNSIGNED)
				syntax_error(*err_loc, "type can not be specified as signed and unsigned");
			if (entry->specifiers & SYM_LONG && entry->specifiers & SYM_SHORT)
				syntax_error(*err_loc, "type can not be specified as short and long");
	}
}

static struct ast_node*
parse_declaration(struct lex_file_buffer *fb, struct sym_table *table)
{
	int id;
	struct ast_node *right, *left;
	struct sym_entry sym_entry;

	sym_entry = parse_sym_type(fb);

	assert(fb->curr_token.type == TOK_IDENTIFIER);

	sym_entry.hash = fb->curr_token.hash;
	id = sym_find_entry(table, sym_entry.hash);
	if (id == -1)
		id = sym_add_entry(table, sym_entry);
	else
		syntax_error(fb->curr_token.err_loc, "redefinition of variabel");

	check_sym_conflicts(&sym_entry, &fb->curr_token.err_loc);	
	sym_print(&sym_entry);

	// inline assignment
	// should also check for assignment list
	if (fb->next_token.type == TOK_ASSIGN) {
		lex_next_token(fb);	
		right = make_ast_node(AST_IDENTIFIER, NULL, NULL, NULL);
		left = parse_expression(fb, table, 0);
		left = make_ast_node(AST_ASSIGN, left, NULL, right);

		if (fb->curr_token.type != TOK_SEMIKOLON)
			syntax_error(fb->curr_token.err_loc, "expected semikolon");

		return left;
	}
	else if (fb->next_token.type == TOK_SEMIKOLON) {
		lex_next_token(fb);

		return NULL;
	}
	else {
		syntax_error(fb->curr_token.err_loc, "expected semikolon");
		return NULL;
	}
}

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
	assert(fb->curr_token.type == TOK_PAREN_OPEN);

	condition_ast = parse_expression(fb, table, 0);

	assert(fb->curr_token.type == TOK_PAREN_CLOSED);
	lex_next_token(fb);

	true_ast = parse_statement(fb, table);
	lex_next_token(fb);
	
	if (fb->next_token.type == TOK_KEY_ELSE) {
		lex_next_token(fb);
		false_ast = parse_else_statement(fb, table);	
	}
	else {
		false_ast = NULL;
	}

	return make_ast_node(AST_IF, condition_ast, true_ast, false_ast);
}

static struct ast_node*
parse_while_loop(struct lex_file_buffer *fb, struct sym_table *table)
{
	struct ast_node *condition_ast, *body_ast;

	assert(fb->curr_token.type == TOK_KEY_WHILE);

	lex_next_token(fb);

	assert(fb->curr_token.type == TOK_PAREN_OPEN);
	printf("got here\n");

	condition_ast = parse_expression(fb, table, 0);

	assert(fb->curr_token.type == TOK_PAREN_CLOSED);
	lex_next_token(fb);

	body_ast = parse_compound_statement(fb, table);

	return make_ast_node(AST_WHILE, condition_ast, NULL, body_ast);	
}

/*
static struct ast_node*
parse_for_loop(struct lex_file_buffer *fb, struct sym_table *table)
{
	struct ast_node *tree, *condition_ast, *body_ast, *preop_ast, *postop_ast;

	lex_next_token(fb);	

	if (fb->curr_token.type != TOK_PAREN_OPEN)
		syntax_error(fb->curr_token.err_loc, "expected \'(\' after for");
	
}
*/

static struct ast_node*
parse_statement(struct lex_file_buffer *fb, struct sym_table *table)
{
	enum ast_type type;
	struct lex_token token = fb->curr_token;
	struct ast_node *tree;

	switch (token.type) {
		case TOK_KEY_IF:
			return parse_if_statement(fb, table);

		case TOK_KEY_ELSE:
			syntax_error(token.err_loc, "no maching if statement");
			tree = NULL;
			break;

		case TOK_BRACE_OPEN:
			return parse_compound_statement(fb, table);

		case TOK_KEY_WHILE:
			return parse_while_loop(fb, table);

		case TOK_IDENTIFIER:
			type = assign_tok_to_ast(fb->next_token.type);
			if (type) {
				lex_next_token(fb);
				tree = parse_assignment(fb, table, type);
			}
			else {
				syntax_warning(token.err_loc, "result unused");
				tree = parse_expression(fb, table, 0);
			}
			break;

		case TOK_KEY_INT:	
		case TOK_KEY_SHORT:
		case TOK_KEY_CHAR:
		case TOK_KEY_FLOAT:
		case TOK_KEY_DOUBLE:
		case TOK_KEY_SIGNED:
		case TOK_KEY_UNSIGNED:
		case TOK_KEY_REGISTER:
		case TOK_KEY_VOLATILE:
		case TOK_KEY_CONST:
		case TOK_KEY_LONG:
		case TOK_KEY_EXTERN:
			tree = parse_declaration(fb, table);
			break;

		default:

			syntax_error(token.err_loc, "internal : parse_statement : %s", lex_tok_str(token.type));
			assert(false);
			break;
	}

	if (fb->curr_token.type != TOK_SEMIKOLON)
		syntax_error(fb->curr_token.err_loc, "expected semikolon");
	else
		lex_next_token(fb);

	return tree;
}

static struct ast_node*
parse_compound_statement(struct lex_file_buffer *fb, struct sym_table *table)
{
	struct ast_node *left = NULL, *tree = NULL;
	assert(fb->curr_token.type == TOK_BRACE_OPEN);
	lex_next_token(fb);

	for (;;) {
		if (fb->curr_token.type == TOK_BRACE_CLOSED) {
			lex_next_token(fb);
			return left;
		}

		tree = parse_statement(fb, table);
		
		if (tree) {
			if (!left)
				left = tree;
			else
				left = make_ast_node(AST_NOP, left, NULL, tree);
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

