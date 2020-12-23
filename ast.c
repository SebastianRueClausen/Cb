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
	"AST_MOD",

	"AST_EQUAL",
	"AST_NOT_EQUAL",
	"AST_LESSER",
	"AST_GREATER",
	"AST_LESSER_EQUAL",
	"AST_GREATER_EQUAL",

	"AST_UNARY_PLUS",
	"AST_UNARY_MINUS",
	"AST_UNARY_NOT",

	"AST_BIT_NOT",

	"AST_PRE_INCREMENT",
	"AST_PRE_DECREMENT",

	"AST_POST_INCREMENT",
	"AST_POST_DECREMENT",

	"AST_DEREF",
	"AST_ADDRESS",

	"AST_PROMOTE",

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
	return "DEBUG";
}

#endif

/* operation tokens used in expressions */
/* @perforamce: make sections lign up with token type for farster,
 * conversion */
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
		case TOK_MOD:
			return AST_MOD;
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

/* since the type info can be represented in different ways in ast nodes, 
 * this function gets the right one dependent on ast_type */
static struct type_info
ast_type_info(struct ast_instance *ast_in, struct ast_node *node)
{
	switch (node->type) {

		case AST_LITERAL:
			return node->literal.type;

		case AST_IDENTIFIER:
			return sym_get_entry(ast_in->sym_table, node->sym_id)->type;

		/* we just assume its some other node as part of and expression,
		 * or an unary node, in which case we also save
		 * the type of what its pointing at in expr_type*/
		default:
			return node->expr_type;
	}
}


/* allocate and create ast node */
inline static struct ast_node*
make_ast_node(struct ast_instance *ast_in, enum ast_type type,
			  struct ast_node *left, struct ast_node *center,
			  struct ast_node *right)
{
	struct ast_node *node;

	node = mem_pool_alloc(&ast_in->pool, sizeof(struct ast_node));
	node->left = left;
	node->center = center;
	node->right	= right;
	node->type = type;

	return node;
}

/* create an */
static struct ast_node*
make_rvalue_node(struct ast_instance *ast_in, const struct lex_token *token)
{
	struct ast_node *node;

	switch (token->type) {
		case TOK_LITERAL:
			node = make_ast_node(ast_in, AST_LITERAL, NULL, NULL, NULL);
			node->literal.type = token->literal.type;
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

	/* sym will always be defined before we use it here */
	if (id == SYM_NOT_FOUND) {
		syntax_error(token->err_loc, "use of undeclared identifier");
	}

	node = make_ast_node(ast_in, AST_IDENTIFIER, NULL, NULL, NULL);
	node->sym_id = id;
	
	return node;
}

/* helper function to create unary nodes */
static struct ast_node*
make_unary_node(struct ast_instance *ast_in, enum ast_type type,
				struct ast_node *node, struct type_info type_info)
{
	struct ast_node *unary_node;

	unary_node = make_ast_node(ast_in, type, node, NULL, NULL);

	/* the type may depent on the type of prefix */
	unary_node->expr_type = type_info;

	return unary_node;
}

/* parse function call, curr token must be on identifier */
static struct ast_node*
parse_function_call(struct ast_instance *ast_in)
{
	struct sym_entry *func;

	/* get current token */
	struct lex_token token = ast_in->lex_in->curr_token;

	/* find the symbol */
	int32_t id = sym_find_id(ast_in->sym_table, token.hash);

	/* make sure that it's declared */
	if (id == SYM_NOT_FOUND) {
		syntax_error(token.err_loc, "undefined function");
	}

	/* get entry */
	func = sym_get_entry(ast_in->sym_table, id);

	/* make sure its a function */
	if (func->kind == SYM_KIND_FUNCTION) {
		syntax_error(token.err_loc, "cannot call variabel");
	}

	/* skip identifier */
	lex_next_token(ast_in->lex_in);

	/* skip '(', we should not have to check */
	lex_next_token(ast_in->lex_in);

	/* */

}

/* peek at next token, and determine if it's a postfix, and
 * if it is we generate the ast tree, and skip the postfix,
 * otherwise return NULL */
static struct ast_node*
parse_postfix(struct ast_instance *ast_in)
{
	struct ast_node *node;

	/* peek at next token */
	struct lex_token token = ast_in->lex_in->next_token;

	switch (token.type) {

		/* function call */
		case TOK_PAREN_OPEN:

			return parse_function_call(ast_in);

		/* post increment */
		case TOK_INCREASE:

			lex_next_token(ast_in->lex_in);

			return make_ast_node(ast_in, AST_POST_INCREMENT, NULL, NULL, NULL);

		/* post decrement */
		case TOK_DECREASE:

			lex_next_token(ast_in->lex_in);

			return make_ast_node(ast_in, AST_POST_DECREMENT, NULL, NULL, NULL);	
		/* member of struct/union */
		case TOK_DOT:
			break;

		/* array access */
		case TOK_BRACKET_OPEN:
			break;

	}
}

/* parses prefix and insures the next token is valid */
static struct ast_node*
parse_prefix(struct ast_instance *ast_in)
{
	struct ast_node *node = NULL;
	struct lex_token token = ast_in->lex_in->curr_token;

	/* remeber type to avoid doing lookup later */
	struct type_info type = NULL_TYPE_INFO;

	/* @todo: type checking */

	/* @refactor: repeating code */
	switch (token.type) {

		/* unary plus */
		case TOK_PLUS:

			token = lex_next_token(ast_in->lex_in);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(ast_in, &token);
				type = sym_get_entry(ast_in->sym_table, node->sym_id)->type;
			} else if (is_rvalue(token.type)) {
				node = make_rvalue_node(ast_in, &token);
				type = node->literal.type;
			} else {
				syntax_error(ast_in->lex_in->last_token.err_loc,
						"unexpected operand for unary operator");
			}

			return make_unary_node(ast_in, AST_UNARY_PLUS, node, type);
	

		/* unary minus */
		case TOK_MINUS:

			token = lex_next_token(ast_in->lex_in);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(ast_in, &token);
				type = sym_get_entry(ast_in->sym_table, node->sym_id)->type;
			} else if (is_rvalue(token.type)) {
				node = make_rvalue_node(ast_in, &token);
				type = node->literal.type;
			} else {
				syntax_error(ast_in->lex_in->last_token.err_loc,
						"unexpected operand for unary operator");
			}

			return make_unary_node(ast_in, AST_UNARY_MINUS, node, type);


		/* unary not*/
		case TOK_NOT:

			token = lex_next_token(ast_in->lex_in);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(ast_in, &token);
				type = sym_get_entry(ast_in->sym_table, node->sym_id)->type;
			} else if (is_rvalue(token.type)) {
				node = make_rvalue_node(ast_in, &token);
				type = node->literal.type;
			} else {
				syntax_error(ast_in->lex_in->last_token.err_loc,
						"unexpected operand for unary operator");
			}

			return make_unary_node(ast_in, AST_UNARY_NOT, node, type);

		/* bitwise not */
		case TOK_NOT_BIT:

			token = lex_next_token(ast_in->lex_in);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(ast_in, &token);
				type = sym_get_entry(ast_in->sym_table, node->sym_id)->type;
			} else if (is_rvalue(token.type)) {
				node = make_rvalue_node(ast_in, &token);
				type = node->literal.type;
			} else {
				syntax_error(ast_in->lex_in->last_token.err_loc,
						"unexpected operand for unary operator");
			}

			return make_unary_node(ast_in, AST_BIT_NOT, node, type);
		

		/* pre increment */
		case TOK_INCREASE:

			token = lex_next_token(ast_in->lex_in);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(ast_in, &token);
				type = sym_get_entry(ast_in->sym_table, node->sym_id)->type;
			} else if (is_rvalue(token.type)) {
				syntax_error(ast_in->lex_in->last_token.err_loc,
						"r-value is not assignable");
			} else {
				syntax_error(ast_in->lex_in->last_token.err_loc, "prefix error");
			}

			return make_unary_node(ast_in, AST_PRE_INCREMENT, node, type);


		/* pre decrement */
		case TOK_DECREASE:

			token = lex_next_token(ast_in->lex_in);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(ast_in, &token);
				type = sym_get_entry(ast_in->sym_table, node->sym_id)->type;
			} else if (is_rvalue(token.type)) {
				syntax_error(ast_in->lex_in->last_token.err_loc,
						"r-value is not assignable");
			} else {
				syntax_error(ast_in->lex_in->last_token.err_loc, "prefix error");
			}
			
			return make_unary_node(ast_in, AST_PRE_DECREMENT, node, type);


		/* address of */
		case TOK_AMPERSAND:

			/* skip '~' */
			token = lex_next_token(ast_in->lex_in);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(ast_in, &token);
				type = sym_get_entry(ast_in->sym_table, node->sym_id)->type;
				/* always on more level of indirection */
				++type.indirection;

			} else if (is_rvalue(token.type)) {
				syntax_error(ast_in->lex_in->last_token.err_loc,
						"can not take address of rvalue");
			} else {
				syntax_error(ast_in->lex_in->last_token.err_loc,
						"expected expression");
			}

			return make_unary_node(ast_in, AST_ADDRESS, node, type);

		/* derefrence */
		case TOK_STAR:

			/* skip '*' */
			token = lex_next_token(ast_in->lex_in);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(ast_in, &token);
				type = sym_get_entry(ast_in->sym_table, node->sym_id)->type;

				/* make sure it's a pointer */
				if (type.indirection == 0) {
					syntax_error(ast_in->lex_in->last_token.err_loc,
						"can not dereference non pointer");
				}

				/* set one level lo*/
				--type.indirection;

			} else if (is_rvalue(token.type)) {
				syntax_error(ast_in->lex_in->last_token.err_loc,
						"can not dereference rvalue");
			} else {
				syntax_error(ast_in->lex_in->last_token.err_loc,
						"expected expression");
			}

			return make_unary_node(ast_in, AST_DEREF, node, type);

		default:
			/* returns NULL if the prefix is invalid */
			return NULL;
	}
}

/* pratt parser precendeces (higher value = higher precendece) */
static const int ast_precedence[_AST_COUNT] =
{
	[AST_UNDEF]				= -1,

	/* primary expression */
	[AST_IDENTIFIER]		= 0,
	[AST_LITERAL]			= 0,

	/* assignment */
	[AST_ASSIGN]			= 1,

	/* additive expression */
	[AST_ADD]				= 2, 
	[AST_MIN]				= 2,

	/* multiplicative expression */
	[AST_MUL]				= 3,
	[AST_DIV]				= 3,
	[AST_MOD]				= 3,

	/* relational expression */
	[AST_EQUAL]				= 4,
	[AST_NOT_EQUAL]			= 4,

	/* comparative expression */
	[AST_LESSER]			= 5,
	[AST_GREATER]			= 5,
	[AST_LESSER_EQUAL]		= 5,
	[AST_GREATER_EQUAL]		= 5,
};

/* ie. parsed from right to left */
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

/* adapt types if required, or throw error if not compat */
static void
expr_type_check(struct ast_instance *ast_in, struct ast_node **left,
		   struct ast_node **right)
{
	enum type_compat compat;

	/* we fetch the compatibility of the righ and left nodes */
	compat = type_compat(ast_type_info(ast_in, *left),
						 ast_type_info(ast_in, *right));

	/* we lookup what to do depent in compat */
	switch (compat) {

		/* if the types a compatible we dont need to do anything */
		case TYPE_COMPAT_COMPAT:
			printf("type compat\n");
			return;

		/* we promote the left or the right */
		case TYPE_COMPAT_PROMOTE_LEFT:
			printf("type widen left\n");
			*left = make_ast_node(ast_in, AST_PROMOTE, *left, NULL, NULL);
			break;

		case TYPE_COMPAT_PROMOTE_RIGHT:
			printf("type widen right\n");
			*right = make_ast_node(ast_in, AST_PROMOTE, *right, NULL, NULL);
			break;

		/* if they are incompatible we throw a syntax error */
		case TYPE_COMPAT_INCOMPAT:
			syntax_error(ast_in->lex_in->curr_token.err_loc,
					"type incompatible in expression");
			break;
	}
}

/* recursive pratt parser */
/* expect that the current token, is the first one of the expression */
static struct ast_node*
parse_expression(struct ast_instance *ast_in, int32_t prev_prec,
				 enum lex_token_type terminator)
{
	struct lex_token token, next_token;
	struct ast_node *left = NULL, *right, *tmp;
	enum ast_type node_type;
	struct type_info expr_type;

	/* get value token */
	token = ast_in->lex_in->curr_token;

	if (token.type == TOK_IDENTIFIER) {
		left = make_lvalue_node(ast_in, &token);
	} else if (is_rvalue(token.type)) {
		left = make_rvalue_node(ast_in, &token);
	} else if (token.type == TOK_PAREN_OPEN) {
		lex_next_token(ast_in->lex_in);
		left = parse_expression(ast_in, 0, TOK_PAREN_CLOSED);
	} else {
		/* the only other valid option is a prefix */
		left = parse_prefix(ast_in);

		if (!left) {
			syntax_error(token.err_loc, "failed parsing prefix");
		}
	}

	assert(left);

	/* the type of the expression is the type of
	 * the left node */
	expr_type = ast_type_info(ast_in, left);

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

		 /* parse the next part of expression */
		lex_next_token(ast_in->lex_in);
		right = parse_expression(ast_in, ast_precedence[node_type], terminator);

		/* check type compat, and modify the type required */
		expr_type_check(ast_in, &left, &right);	

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

		/* we set the type of the expression */
		left->expr_type = expr_type;

		if (ast_in->lex_in->curr_token.type == terminator) {
			return left;
		}

		node_type = op_tok_to_ast(ast_in->lex_in->curr_token.type);
		if (!node_type) {
			syntax_error(ast_in->lex_in->curr_token.err_loc,
					"expected expression");
		}
	}

	printf("exited expression with current token %s\n",
			lex_tok_debug_str(ast_in->lex_in->curr_token.type));
	return left;
}

/* just used by the function below to call errors */
inline static const char*
tok_err_str(enum lex_token_type type)
{
	switch (type) {
		case TOK_SEMIKOLON:
			return ";";

		case TOK_PAREN_OPEN:
			return "(";

		case TOK_PAREN_CLOSED:
			return ")";

		case TOK_IDENTIFIER:
			return "identifier";

		default:
			return "internal error";
	}
}

inline static void
assert_token(struct ast_instance *ast_in, enum lex_token_type type)
{
	if (ast_in->lex_in->curr_token.type != type) {
		syntax_error(ast_in->lex_in->curr_token.err_loc, "expected '%s'",
				tok_err_str(type));
	}
}

/* parse single token, and throw error if different token is found */
inline static void
parse_token(struct ast_instance *ast_in, enum lex_token_type type)
{
	assert_token(ast_in, type);

	lex_next_token(ast_in->lex_in);
}

static struct ast_node*
parse_compound_statement(struct ast_instance *ast_in);

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

	type->spec |= spec;
}

/* parse a list of symbol specifier */
static struct type_info
parse_type(struct ast_instance *ast_in)
{
	struct type_info type = NULL_TYPE_INFO;

	for (;;) {
		switch (ast_in->lex_in->curr_token.type) {

			/* primitive */
			case TOK_KEY_INT:
				set_type_prim(&type, TYPE_PRIM_INT,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_FLOAT:
				set_type_prim(&type, TYPE_PRIM_FLOAT,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_CHAR:
				set_type_prim(&type, TYPE_PRIM_CHAR,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_DOUBLE:
				set_type_prim(&type, TYPE_PRIM_DOUBLE,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			/* type specifiers */
			case TOK_KEY_CONST:
				set_type_spec(&type, TYPE_SPEC_CONST,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_STATIC:
				set_type_spec(&type, TYPE_SPEC_STATIC,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_UNSIGNED:
				set_type_spec(&type, TYPE_SPEC_UNSIGNED,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_SIGNED:
				set_type_spec(&type, TYPE_SPEC_SIGNED,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_SHORT:
				set_type_spec(&type, TYPE_SPEC_SHORT,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_LONG:
				set_type_spec(&type, TYPE_SPEC_LONG,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_REGISTER:
				set_type_spec(&type, TYPE_SPEC_REGISTER,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_VOLATILE:
				set_type_spec(&type, TYPE_SPEC_VOLATILE,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			case TOK_KEY_EXTERN:
				set_type_spec(&type, TYPE_SPEC_EXTERN,
						&ast_in->lex_in->curr_token.err_loc);
				break;

			/* return when the */
			default:
				return type;
		}

		lex_next_token(ast_in->lex_in);
	}
}

static struct ast_node*
parse_function_definition(struct ast_instance *ast_in, int32_t id);

static struct ast_node*
parse_function_declaration(struct ast_instance *ast_in, int32_t id)
{
	/* @note skip params for now */
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
	int64_t sym_hash;
	struct ast_node *right, *left;
	struct sym_entry sym_entry;
	enum lex_token_type tok_type;

	/* loops through type specifiers */
	sym_entry.type = parse_type(ast_in);

	/* loop through stars to get indirection */
	tok_type = ast_in->lex_in->curr_token.type;

	while (tok_type == TOK_STAR) {
		++sym_entry.type.indirection;
		tok_type = lex_next_token(ast_in->lex_in).type;
	}

	/* check for conflicts */
	type_check_conflicts(sym_entry.type, &ast_in->lex_in->curr_token.err_loc);	

	/* the next token after specifiers must be an identifier */
	parse_token(ast_in, TOK_IDENTIFIER);

	/* get sym id */
	sym_hash = ast_in->lex_in->curr_token.hash;
	id = sym_find_id(table, sym_hash);

	/* make sure variable isnt declared already */
	if (id != SYM_NOT_FOUND) {
		syntax_error(ast_in->lex_in->curr_token.err_loc,
				"redefinition of variabel");
	}

	/* add to symbol table */
	id = sym_add_entry(table, sym_entry, sym_hash);


	/* check for inline assignment an such */
	switch (ast_in->lex_in->curr_token.type) {

		/* inline assignment */
		case TOK_ASSIGN:

			/* skip '=' */
			lex_next_token(ast_in->lex_in);

			/* since we assign it a value it must be a varable */
			sym_get_entry(ast_in->sym_table, id)->kind = SYM_KIND_VARIABLE;	

			/* make an identifier ast node */
			right = make_ast_node(ast_in, AST_IDENTIFIER, NULL, NULL, NULL);

			/* parse the expression */
			left = parse_expression(ast_in, 0, TOK_SEMIKOLON);

			/* skip semikolon */
			lex_next_token(ast_in->lex_in);

			/* glue together expression */
			left = make_ast_node(ast_in, AST_ASSIGN, left, NULL, right);

			return left;


		/* function */
		case TOK_PAREN_OPEN:
			
			/* symbol must be a function */
			sym_get_entry(ast_in->sym_table, id)->kind = SYM_KIND_FUNCTION;	

			left = parse_function_declaration(ast_in, id);	

			return left;


		/* undeclared  */	
		default:

			/* skip semikolon */
			parse_token(ast_in, TOK_SEMIKOLON);	
			
			/* must be a variable */
			sym_get_entry(ast_in->sym_table, id)->kind = SYM_KIND_VARIABLE;	
			
			/* we return NULL since this doesnt produce any code */
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

	/* skip 'if' token */
	lex_next_token(ast_in->lex_in);

	/* skip opening '(' */
	parse_token(ast_in, TOK_PAREN_OPEN);

	/* parse conditional expression */
	condition_ast = parse_expression(ast_in, 0, TOK_PAREN_CLOSED);

	/* skip closing ')' */
	lex_next_token(ast_in->lex_in);

	/* parse the body */
	true_ast = parse_statement(ast_in);

	if (ast_in->lex_in->curr_token.type == TOK_KEY_ELSE) {
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

	/* skip 'while' token */	
	lex_next_token(ast_in->lex_in);

	/* skip the '(' */
	parse_token(ast_in, TOK_PAREN_OPEN);

	/* parse the conditional */
	condition_ast = parse_expression(ast_in, 0, TOK_PAREN_CLOSED);

	/* skip closing ')' */
	lex_next_token(ast_in->lex_in);

	/* parse the body */
	body_ast = parse_statement(ast_in);

	/* glue body and conditional together with a while ast */
	return make_ast_node(ast_in, AST_WHILE, condition_ast, NULL, body_ast);

}

static struct ast_node*
parse_for_loop(struct ast_instance *ast_in)
{
	struct ast_node *tree, *body, *cond, *pre_op, *post_op;

	/* skip opening 'for' token */
	lex_next_token(ast_in->lex_in);

	/* skip the '(' */
	parse_token(ast_in, TOK_PAREN_OPEN);

	/* parse preop expression */
	pre_op = parse_expression(ast_in, 0, TOK_SEMIKOLON);
	lex_next_token(ast_in->lex_in);

	/* parse conditional statement */
	cond = parse_expression(ast_in, 0, TOK_SEMIKOLON);
	lex_next_token(ast_in->lex_in);

	/* parse post op */	
	post_op = parse_expression(ast_in, 0, TOK_PAREN_CLOSED);
	lex_next_token(ast_in->lex_in);

	/* parse statement */
	body = parse_statement(ast_in);

	/* glue the body and post op so we does the postop after the body */	
	tree = make_ast_node(ast_in, AST_NOP, body, NULL, post_op);

	/* we make a while loop with the body and cond */
	tree = make_ast_node(ast_in, AST_WHILE, cond, NULL, tree);

	/* glue the preop and while loop so we does the preop before the loop */
	tree = make_ast_node(ast_in, AST_NOP, pre_op, NULL, tree);

	return tree;
}

/* creates an ast tree from a single statement, which can include compound statements */
/* expect the current token to be the first of the statement */
struct ast_node*
parse_statement(struct ast_instance *ast_in)
{
	struct lex_token token = ast_in->lex_in->curr_token;
	struct ast_node *tree;

	switch (token.type) {
		case TOK_KEY_IF:
			return parse_if_statement(ast_in);

		case TOK_KEY_FOR:
			return parse_for_loop(ast_in);

		case TOK_KEY_WHILE:
			return parse_while_loop(ast_in);

		case TOK_KEY_ELSE:
			syntax_error(token.err_loc, "no maching if statement");
			return NULL;

		case TOK_BRACE_OPEN:
			return parse_compound_statement(ast_in);

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
			return parse_declaration(ast_in, ast_in->sym_table);

		default:
			tree = parse_expression(ast_in, 0, TOK_SEMIKOLON);

			/* skip semikolon */
			lex_next_token(ast_in->lex_in);

			return tree;
	}
}

/* loops through all statements in a compound statement and creates an ast tree */
static struct ast_node*
parse_compound_statement(struct ast_instance *ast_in)
{
	struct ast_node *left = NULL, *tree = NULL;

	/* skip opening '{' */
	lex_next_token(ast_in->lex_in);

	/* parse all statements until we get a closed brace */
	for (;;) {
		if (ast_in->lex_in->curr_token.type == TOK_BRACE_CLOSED) {
			lex_next_token(ast_in->lex_in);
			return left;
		}

		/* parse_statement mat call parse_comound_statement,
		 * so it is called recursively in nested comound statements */
		tree = parse_statement(ast_in);
		
		if (tree) {
			/* if its the first statement */
			if (!left) {
				left = tree;
			} else {
				/* glue together with prev statements */
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
/*  ============================ PUBLIC =============================== */
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

/* @debug: make this easier to read */
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

