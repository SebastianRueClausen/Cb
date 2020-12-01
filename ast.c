#include "ast.h"
#include "lex.h"
#include "def.h"
#include "sym.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

/*
 * We parse the token inputstream, and parse them into a more abstract
 * and lower level abstract syntax tree,
 * which make sure the syntax is correct, expression happen in the right
 * order and prepares the syntax for optimization and code generation.
 */

#define DEBUG 1 

#ifdef DEBUG

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

	"AST_UNARY_PLUS",
	"AST_UNARY_MINUS",

	"AST_PRE_INCREMENT",
	"AST_PRE_DECREMENT",

	"AST_DEREF",
	"AST_ADDRESS",

	"AST_ASSIGN",

	"AST_LITERAL",
	
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

#else

const char*
ast_to_str(enum ast_type type)
{
	type = 0;
	return "DEBUG";
}

#endif


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
		case TOK_ASSIGN:
			return AST_ASSIGN;
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
		case TOK_LITERAL:
			return true;
		default:
			return false;
	}
}

/* allocate and create ast node */
static struct ast_node*
make_ast_node(struct ast_instance *ast_in, enum ast_type type, struct ast_node *left,
			  struct ast_node *center, struct ast_node *right)
{
	struct ast_node *node;

	node = mem_pool_alloc(&ast_in->pool, sizeof(struct ast_node));
	node->left = left;
	node->center = center;
	node->right	= right;
	node->type = type;

	return node;
}

/* determine the datatype and create a node*/
static struct ast_node*
make_rvalue_node(struct ast_instance *ast_in, const struct lex_token *token)
{
	struct ast_node *node;

	switch (token->type) {
		case TOK_LITERAL:
			node = make_ast_node(ast_in, AST_LITERAL, NULL, NULL, NULL);
			node->literal.value = token->literal.value;
			break;

		default:
			assert(false);
			break;
	}

	node->value_type = AST_RVALUE;	

	return node;
}

/* assumes the symbol already exists and creates a node */
static struct ast_node*
make_lvalue_node(struct ast_instance *ast_in, const struct lex_token *token)
{
	int32_t id;
	struct ast_node *node;

	id = sym_find_id(ast_in->sym_table, token->hash);

	if (id == SYM_NOT_FOUND) {
		syntax_error(token->err_loc, "use of undeclared identifier");
	}

	node = make_ast_node(ast_in, AST_IDENTIFIER, NULL, NULL, NULL);
	node->sym_id = id;
	
	return node;
}

/* parses prefix and insures the next token is valid */
static struct ast_node*
parse_prefix(struct ast_instance *ast_in)
{
	struct ast_node *node = NULL;

	switch (ast_in->lex_in->curr_token.type) {
		case TOK_PLUS:
			lex_next_token(ast_in->lex_in);

			if (ast_in->lex_in->curr_token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(ast_in, &ast_in->lex_in->curr_token);
			} else if (is_rvalue(ast_in->lex_in->curr_token.type)) {
				node = make_rvalue_node(ast_in, &ast_in->lex_in->curr_token);
			} else {
				syntax_error(ast_in->lex_in->last_token.err_loc,
						"unexpected operand for unary operator");
			}

			return make_ast_node(ast_in, AST_UNARY_PLUS, node, NULL, NULL);
			
		case TOK_MINUS:
			lex_next_token(ast_in->lex_in);

			if (ast_in->lex_in->curr_token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(ast_in, &ast_in->lex_in->curr_token);
			} else if (is_rvalue(ast_in->lex_in->curr_token.type)) {
				node = make_rvalue_node(ast_in, &ast_in->lex_in->curr_token);
			} else {
				syntax_error(ast_in->lex_in->last_token.err_loc,
						"unexpected operand for unary operator");
			}

			return make_ast_node(ast_in, AST_UNARY_MINUS, node, NULL, NULL);

		case TOK_INCREASE:
			lex_next_token(ast_in->lex_in);

			if (ast_in->lex_in->curr_token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(ast_in, &ast_in->lex_in->curr_token);
			} else if (is_rvalue(ast_in->lex_in->curr_token.type)) {
				syntax_error(ast_in->lex_in->last_token.err_loc, "r-value is not assignable");
			} else {
				syntax_error(ast_in->lex_in->last_token.err_loc, "prefix error");
			}

			return make_ast_node(ast_in, AST_PRE_INCREMENT, node, NULL, NULL);

		case TOK_DECREASE:
			lex_next_token(ast_in->lex_in);

			if (ast_in->lex_in->curr_token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(ast_in, &ast_in->lex_in->curr_token);
			} else if (is_rvalue(ast_in->lex_in->curr_token.type)) {
				syntax_error(ast_in->lex_in->last_token.err_loc, "r-value is not assignable");
			} else {
				syntax_error(ast_in->lex_in->last_token.err_loc, "prefix error");
			}
			
			return make_ast_node(ast_in, AST_PRE_DECREMENT, node, NULL, NULL);

		case TOK_AMPERSAND:
			lex_next_token(ast_in->lex_in);

			if (ast_in->lex_in->curr_token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(ast_in, &ast_in->lex_in->curr_token);
			} else if (is_rvalue(ast_in->lex_in->curr_token.type)) {
				syntax_error(ast_in->lex_in->last_token.err_loc,
						"can not take address of rvalue");
			} else {
				syntax_error(ast_in->lex_in->last_token.err_loc,
						"expected expression");
			}

			return make_ast_node(ast_in, AST_ADDRESS, node, NULL, NULL);

		case TOK_STAR:
			lex_next_token(ast_in->lex_in);

			if (ast_in->lex_in->curr_token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(ast_in, &ast_in->lex_in->curr_token);
			} else if (is_rvalue(ast_in->lex_in->curr_token.type)) {
				syntax_error(ast_in->lex_in->last_token.err_loc,
						"can not dereference rvalue");
			} else {
				syntax_error(ast_in->lex_in->last_token.err_loc,
						"expected expression");
			}

			return make_ast_node(ast_in, AST_DEREF, node, NULL, NULL);

		default:
			return NULL;
	}
}

/* pratt parser precendeces */
static const int ast_precedence[_AST_COUNT] =
{
	[AST_UNDEF]				= -1,

	[AST_IDENTIFIER]		= 0,
	[AST_LITERAL]			= 0,

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

static bool
is_right_associavity(enum ast_type type)
{
	switch (type) {
		case AST_ASSIGN:
			return true;
		default:
			return false;
	}
}

/* recursive pratt parser */
static struct ast_node*
parse_expression(struct ast_instance *ast_in, int32_t prev_prec,
				 enum lex_token_type terminator)
{
	struct lex_token token, next_token;
	struct ast_node *left = NULL, *right, *tmp;
	enum ast_type node_type;

	/* get value token */
	token = lex_next_token(ast_in->lex_in);

	if (token.type == TOK_IDENTIFIER) {
		left = make_lvalue_node(ast_in, &token);
	} else if (is_rvalue(token.type)) {
		left = make_rvalue_node(ast_in, &token);
	} else if (token.type == TOK_PAREN_OPEN) {
		left = parse_expression(ast_in, 0, TOK_PAREN_CLOSED);
	} else {
		/* the only other valid option is a prefix */
		left = parse_prefix(ast_in);

		if (!left) {
			syntax_error(token.err_loc, "expected expression");
		}
	}

	/* get op token */
	next_token = lex_next_token(ast_in->lex_in);
	if (next_token.type == terminator) {
		return left;
	}

	node_type = op_tok_to_ast(next_token.type);
	if (!node_type) {
		syntax_error(err_loc(ast_in->lex_in), "expected expression");
	}

	/*
	 * if the prev token precedence is lower than the prev token
	 * we return (go up a level in the tree)
	*/
	while (prev_prec < ast_precedence[node_type] ||
		   (is_right_associavity(node_type) &&
		   prev_prec == ast_precedence[node_type])) {

		right = parse_expression(ast_in, ast_precedence[node_type], terminator);

		/* we must handle assignment differently */
		if (ast_in->lex_in->curr_token.type == TOK_ASSIGN) {
			/* right must be an rvalue when assigning */
			right->value_type = AST_RVALUE;

			/* swap the right and left, so the right branch
			 * will be generated first */
			tmp		= left;	
			left	= right;
			right	= tmp;
		} else {

			/* since we arent doing assignment, both sides must be
			 * rvalues */
			left->value_type = AST_RVALUE;
			right->value_type = AST_LVALUE;
		}

		left = make_ast_node(ast_in, node_type, left, NULL, right);

		if (ast_in->lex_in->curr_token.type == terminator) {
			return left;
		}

		node_type = op_tok_to_ast(ast_in->lex_in->curr_token.type);
		if (!node_type) {
			syntax_error(ast_in->lex_in->curr_token.err_loc,
					"expected expression");
		}
	}

	return left;
}

static struct ast_node*
parse_compound_statement(struct ast_instance *ast_in);

// TODO should be declared static vvvvvvvvvvvvvvv
/*
static struct ast_node*
parse_statement(struct lex_instance *li, struct sym_table *table);
*/

// TODO this should be part of the expression
static struct ast_node*
parse_assignment(struct ast_instance *ast_in, struct sym_table *table,
				 enum ast_type type)
{
	struct ast_node *left, *right;
	int32_t id;

	if (ast_in->lex_in->last_token.type != TOK_IDENTIFIER) {
		syntax_error(ast_in->lex_in->last_token.err_loc, "assigning to rvalue");
	}

	id = sym_find_id(table, ast_in->lex_in->last_token.hash);

	if (id == SYM_NOT_FOUND) {
		syntax_error(ast_in->lex_in->last_token.err_loc,
				"use of undeclated identifier");
	}

	right = make_ast_node(ast_in, AST_IDENTIFIER, NULL, NULL, NULL);

	left = parse_expression(ast_in, 0, TOK_SEMIKOLON);
	assert(ast_in->lex_in->curr_token.type == TOK_SEMIKOLON);

	left = make_ast_node(ast_in, type, left, NULL, right);

	return left;	
}

/* asserts that we doesnt set a type prim more than once */
inline static void
set_type_prim(struct type_info *type, enum type_prim prim,
			  const struct err_location *err_loc)
{
	if (type->prim != TYPE_PRIM_NONE) {
		syntax_error(*err_loc, "duplicate type specifier");
	}

	type->prim = prim;
}

/* check that we doesnt set a type spec more than once,
 * we check for conflicts such as signed and unsigned when we are done */
inline static void
set_type_spec(struct type_info *type, enum type_spec spec,
			  const struct err_location *err_loc)
{
	if (type->spec & spec) {
		syntax_error(*err_loc, "duplicate type specifier");
	}

	type->spec &= spec;
}

/* parse a list of symbol specifier */
static struct sym_entry
parse_sym_type(struct ast_instance *ast_in)
{
	struct sym_entry sym_entry;

	sym_entry.type.spec = 0;
	sym_entry.type.prim = TYPE_PRIM_NONE;
	sym_entry.type.indirection = 0;
	
	for (;;) {
		switch (ast_in->lex_in->curr_token.type) {
			case TOK_KEY_INT:
				set_type_prim(&sym_entry.type, TYPE_PRIM_INT,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_FLOAT:
				set_type_prim(&sym_entry.type, TYPE_PRIM_FLOAT,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_CHAR:
				set_type_prim(&sym_entry.type, TYPE_PRIM_CHAR,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_DOUBLE:
				set_type_prim(&sym_entry.type, TYPE_PRIM_DOUBLE,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_CONST:
				set_type_spec(&sym_entry.type, TYPE_SPEC_CONST,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_STATIC:
				set_type_spec(&sym_entry.type, TYPE_SPEC_STATIC,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_UNSIGNED:
				set_type_spec(&sym_entry.type, TYPE_SPEC_UNSIGNED,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_SIGNED:
				set_type_spec(&sym_entry.type, TYPE_SPEC_SIGNED,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_SHORT:
				set_type_spec(&sym_entry.type, TYPE_SPEC_SHORT,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_LONG:
				set_type_spec(&sym_entry.type, TYPE_SPEC_LONG,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_REGISTER:
				set_type_spec(&sym_entry.type, TYPE_SPEC_REGISTER,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_VOLATILE:
				set_type_spec(&sym_entry.type, TYPE_SPEC_VOLATILE,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_EXTERN:
				set_type_spec(&sym_entry.type, TYPE_SPEC_EXTERN,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			default:
				return sym_entry;
		}

		lex_next_token(ast_in->lex_in);
	}
}

static struct ast_node*
parse_function_definition(struct ast_instance *ast_in, int32_t id);

static struct ast_node*
parse_function_declaration(struct ast_instance *ast_in, int32_t id)
{
	// @note skip params for now	
	while (ast_in->lex_in->curr_token.type != TOK_PAREN_CLOSED) {
		lex_next_token(ast_in->lex_in);
	}

	if (ast_in->lex_in->curr_token.type == TOK_SEMIKOLON) {
		return NULL;
	} else if (ast_in->lex_in->curr_token.type == TOK_BRACE_OPEN) {
		return parse_function_definition(ast_in, id);		
	} else {
		syntax_error(ast_in->lex_in->curr_token.err_loc, "expeced semikolon");
		assert(false);
	}
}

/* called when we get a type specifier token */
static struct ast_node*
parse_declaration(struct ast_instance *ast_in, struct sym_table *table)
{
	int32_t id;
	struct ast_node *right, *left;
	struct sym_entry sym_entry;
	enum lex_token_type tok_type;

	/* loops through type specifiers */
	sym_entry = parse_sym_type(ast_in);

	/* loop through stars to get indirection */
	tok_type = ast_in->lex_in->curr_token.type;

	while (tok_type == TOK_STAR) {
		++sym_entry.type.indirection;
		tok_type = lex_next_token(ast_in->lex_in).type;
	}

	/* the next token after specifiers must be an identifier */
	if (ast_in->lex_in->curr_token.type != TOK_IDENTIFIER) {
		syntax_error(ast_in->lex_in->curr_token.err_loc,
				"variabel must have a name");
	}

	sym_entry.hash = ast_in->lex_in->curr_token.hash;
	id = sym_find_id(table, sym_entry.hash);

	if (id == SYM_NOT_FOUND) {
		id = sym_add_entry(table, sym_entry);
	} else {
		syntax_error(ast_in->lex_in->curr_token.err_loc, "redefinition of variabel");
	}

	type_check_conflicts(sym_entry.type, &ast_in->lex_in->curr_token.err_loc);	

	// inline assignment
	// TODO should also check for assignment list
	if (ast_in->lex_in->next_token.type == TOK_ASSIGN) {
		lex_next_token(ast_in->lex_in);	

		sym_get_entry(ast_in->sym_table, id)->kind = SYM_KIND_VARIABLE;	

		right = make_ast_node(ast_in, AST_IDENTIFIER, NULL, NULL, NULL);
		left = parse_expression(ast_in, 0, TOK_SEMIKOLON);
		left = make_ast_node(ast_in, AST_ASSIGN, left, NULL, right);

		if (ast_in->lex_in->curr_token.type != TOK_SEMIKOLON) {
			syntax_error(ast_in->lex_in->curr_token.err_loc, "expected semikolon");
		}

		return left;

	} else if (ast_in->lex_in->next_token.type == TOK_PAREN_OPEN) {
		sym_get_entry(ast_in->sym_table, id)->kind = SYM_KIND_FUNCTION;	
		return parse_function_declaration(ast_in, id);
	} else if (ast_in->lex_in->next_token.type == TOK_SEMIKOLON) {
		lex_next_token(ast_in->lex_in);
		sym_get_entry(ast_in->sym_table, id)->kind = SYM_KIND_VARIABLE;	
		return NULL;
	} else {
		syntax_error(ast_in->lex_in->curr_token.err_loc, "expected semikolon");
		return NULL;
	}
}

static struct ast_node*
parse_else_statement(struct ast_instance *ast_in)
{
	lex_next_token(ast_in->lex_in);
	return parse_statement(ast_in);
}

static struct ast_node*
parse_if_statement(struct ast_instance *ast_in)
{
	struct ast_node *condition_ast, *true_ast, *false_ast;

	assert(ast_in->lex_in->curr_token.type == TOK_KEY_IF);	

	lex_next_token(ast_in->lex_in);
	assert(ast_in->lex_in->curr_token.type == TOK_PAREN_OPEN);

	condition_ast = parse_expression(ast_in, 0, TOK_PAREN_CLOSED);

	lex_next_token(ast_in->lex_in);

	true_ast = parse_statement(ast_in);
	lex_next_token(ast_in->lex_in);
	
	if (ast_in->lex_in->next_token.type == TOK_KEY_ELSE) {
		lex_next_token(ast_in->lex_in);
		false_ast = parse_else_statement(ast_in);
	}
	else {
		false_ast = NULL;
	}

	return make_ast_node(ast_in, AST_IF, condition_ast, true_ast, false_ast);
}

static struct ast_node*
parse_while_loop(struct ast_instance *ast_in)
{
	struct ast_node *condition_ast, *body_ast;

	assert(ast_in->lex_in->curr_token.type == TOK_KEY_WHILE);

	lex_next_token(ast_in->lex_in);

	assert(ast_in->lex_in->curr_token.type == TOK_PAREN_OPEN);
	condition_ast = parse_expression(ast_in, 0, TOK_PAREN_CLOSED);

	assert(ast_in->lex_in->curr_token.type == TOK_PAREN_CLOSED);
	lex_next_token(ast_in->lex_in);

	body_ast = parse_compound_statement(ast_in);

	return make_ast_node(ast_in, AST_WHILE, condition_ast, NULL, body_ast);	
}

/*
static struct ast_node*
parse_for_loop(struct lex_instance *li, struct sym_table *table)
{
	struct ast_node *tree, *condition_ast, *body_ast, *preop_ast, *postop_ast;

	lex_next_token(li);	

	if (li->curr_token.type != TOK_PAREN_OPEN) {
		syntax_error(li->curr_token.err_loc, "expected \'(\' after for");
	}
	
}
*/

/* creates an ast tree from a single statement, which can include compound statements */
struct ast_node*
parse_statement(struct ast_instance *ast_in)
{
	enum ast_type type;
	struct lex_token token = ast_in->lex_in->curr_token;
	struct ast_node *tree;

	switch (token.type) {
		case TOK_KEY_IF:
			return parse_if_statement(ast_in);

		case TOK_KEY_ELSE:
			syntax_error(token.err_loc, "no maching if statement");
			tree = NULL;
			break;

		case TOK_BRACE_OPEN:
			return parse_compound_statement(ast_in);

		case TOK_KEY_WHILE:
			return parse_while_loop(ast_in);

		case TOK_IDENTIFIER:
			/*
			type = assign_tok_to_ast(ast_in->lex_in->next_token.type);
			if (type) {
				lex_next_token(ast_in->lex_in);
				tree = parse_assignment(ast_in, ast_in->sym_table, type);
			} else {
				syntax_warning(token.err_loc, "result unused");
				tree = parse_expression(ast_in, 0, TOK_SEMIKOLON);
			}
			*/

			tree = parse_expression(ast_in, 0, TOK_SEMIKOLON);
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
			tree = parse_declaration(ast_in, ast_in->sym_table);
			break;

		default:
			syntax_error(token.err_loc, "internal : parse_statement : %s",
					lex_tok_str(token.type));
			assert(false);
			break;
	}

	if (ast_in->lex_in->curr_token.type != TOK_SEMIKOLON) {
		syntax_error(ast_in->lex_in->curr_token.err_loc, "expected semikolon");
	} else {
		lex_next_token(ast_in->lex_in);
	}

	return tree;
}

/* loops through all statements in a compound statement and creates an ast tree */
static struct ast_node*
parse_compound_statement(struct ast_instance *ast_in)
{
	struct ast_node *left = NULL, *tree = NULL;

	assert(ast_in->lex_in->curr_token.type == TOK_BRACE_OPEN);

	lex_next_token(ast_in->lex_in);

	for (;;) {
		if (ast_in->lex_in->curr_token.type == TOK_BRACE_CLOSED) {
			lex_next_token(ast_in->lex_in);
			return left;
		}

		tree = parse_statement(ast_in);
		
		if (tree) {
			if (!left) {
				left = tree;
			} else {
				left = make_ast_node(ast_in, AST_NOP, left, NULL, tree);
			}
		}
	}
}

static struct ast_node*
parse_function_definition(struct ast_instance *ast_in, int32_t id)
{
	assert(ast_in->lex_in->curr_token.type == TOK_BRACE_OPEN);
	id = 0;

	return parse_compound_statement(ast_in);	
}

/* ===================================================================== */

struct ast_instance
ast_create_instance(struct lex_instance *lex_in, struct sym_table *table)
{
	const struct ast_instance ast_in =
	{
		.lex_in				= lex_in,
		.pool				= mem_pool_create(1024),
		.sym_table			= table
	};

	return ast_in;
}

void
ast_destroy_instance(struct ast_instance *ast_in)
{
	ast_in->lex_in = NULL;
	ast_in->tree = NULL;
	mem_pool_destroy(&ast_in->pool);
}

void
ast_print_tree(struct ast_node *node, uint32_t level)
{
	uint32_t i;

	++level;
	printf("%s\n", ast_to_str(node->type));
	if (node->left) {
		for (i = 0; i < level; ++i) {
			printf("\t");
		}
		ast_print_tree(node->left, level);			
	}
	
	if (node->center) {
		for (i = 0; i < level; ++i) {
			printf("\t");
		}
		ast_print_tree(node->center, level);			
	}

	if (node->right) {
		for (i = 0; i < level; ++i) {
			printf("\t");
		}
		ast_print_tree(node->right, level);
	}
}

void
ast_print_tree_postorder(struct ast_node *node)
{
	if (!node) {
		return;
	}

	if (node->left) {
		ast_print_tree_postorder(node->left);
	}

	if (node->center) {
		ast_print_tree_postorder(node->center);
	}

	if (node->right) {
		ast_print_tree_postorder(node->right);
	}

	printf("%s\n", ast_to_str(node->type));

}


void
ast_test()
{
	struct lex_instance lex_in;
	struct ast_instance ast_in;
	struct sym_table table;

	lex_in = lex_create_instance("../test/test4.c");
	sym_create_table(&table, 10);
	lex_next_token(&lex_in);

	ast_in = ast_create_instance(&lex_in, &table);

	ast_in.tree = parse_statement(&ast_in);
	ast_print_tree(ast_in.tree, 0);

	ast_destroy_instance(&ast_in);
	lex_destroy_instance(&lex_in);
	sym_destroy_table(&table);

}

