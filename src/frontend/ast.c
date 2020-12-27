#include "frontend.h"

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
ast_to_str(ast_type_t type)
{
	return ast_str[type];
}

#else

const char*
ast_to_str(ast_type_t type)
{
	return "DEBUG";
}

#endif

/* operation tokens used in expressions */
/* @perforamce: make sections lign up with token type for farster,
 * conversion */
static ast_type_t
op_tok_to_ast(lex_token_type_t tok)
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
is_rvalue(lex_token_type_t tok)
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
static type_info_t
ast_get_type_info(frontend_t *f, ast_node_t *node)
{
	switch (node->type) {

		case AST_LITERAL:
			return node->literal.type;

		case AST_IDENTIFIER:
			return ((sym_global_t*)sym_get_entry(&f->sym_table, node->sym_id))->type;

		/* we just assume its some other node as part of and expression,
		 * or an unary node, in which case we also save
		 * the type of what its pointing at in expr_type*/
		default:
			return node->expr_type;
	}
}


/* allocate and create ast node */
inline static ast_node_t*
make_ast_node(frontend_t *f, ast_type_t type,
			  ast_node_t *left, ast_node_t *center,
			  ast_node_t *right)
{
	ast_node_t *node	= mem_pool_alloc(&f->pool, sizeof(ast_node_t));
	node->left			= left;
	node->center		= center;
	node->right			= right;
	node->type			= type;

	return node;
}

/* create an */
inline static ast_node_t*
make_rvalue_node(frontend_t *f, const lex_token_t *token)
{
	ast_node_t *node;

	switch (token->type) {

		case TOK_LITERAL:
			node = make_ast_node(f, AST_LITERAL, NULL, NULL, NULL);
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
inline static ast_node_t*
make_lvalue_node(frontend_t *f, const lex_token_t *token)
{
	sym_id_t id;
	ast_node_t *node;

	id = sym_find_id(&f->sym_table, token->hash);

	/* sym will always be defined before we use it here */
	if (id == SYM_ID_NULL) {
		syntax_error(token->err_loc, "use of undeclared identifier");
	}

	node = make_ast_node(f, AST_IDENTIFIER, NULL, NULL, NULL);
	node->sym_id = id;
	
	return node;
}

/* helper function to create unary nodes */
inline static ast_node_t*
make_unary_node(frontend_t *f, ast_type_t type,
				ast_node_t *node, type_info_t type_info)
{
	ast_node_t *unary_node;

	unary_node = make_ast_node(f, type, node, NULL, NULL);

	/* the type may depent on the type of prefix */
	unary_node->expr_type = type_info;

	return unary_node;
}

/* parse function call, curr token must be on identifier */
static ast_node_t*
parse_function_call(frontend_t *f)
{
	sym_global_t *func;

	/* get current token */
	lex_token_t token = f->lex.curr_token;

	/* find the symbol */
	int32_t id = sym_find_id(&f->sym_table, token.hash);

	/* make sure that it's declared */
	if (id == SYM_ID_NULL) {
		syntax_error(token.err_loc, "undefined function");
	}

	/* get entry */
	func = sym_get_global(&f->sym_table, id);

	/* make sure its a function */
	if (func->kind != SYM_GLOBAL_KIND_FUNCTION) {
		syntax_error(token.err_loc, "cannot call variabel");
	}

	/* skip identifier */
	lex_next_token(f);

	/* skip '(', we should not have to check */
	lex_next_token(f);

	/*  */
	
}

/* peek at next token, and determine if it's a postfix, and
 * if it is we generate the ast tree, and skip the postfix,
 * otherwise return NULL */
static ast_node_t*
parse_postfix(frontend_t *f)
{
	ast_node_t *node;

	/* peek at next token */
	lex_token_t token = f->lex.next_token;

	switch (token.type) {

		/* function call */
		case TOK_PAREN_OPEN:

			return parse_function_call(f);

		/* post increment */
		case TOK_INCREASE:

			lex_next_token(f);

			return make_ast_node(f, AST_POST_INCREMENT, NULL, NULL, NULL);

		/* post decrement */
		case TOK_DECREASE:

			lex_next_token(f);

			return make_ast_node(f, AST_POST_DECREMENT, NULL, NULL, NULL);	

		/* member of struct/union */
		case TOK_DOT:
			break;

		/* array access */
		case TOK_BRACKET_OPEN:
			break;

		default:
			assert(false);

	}
}

/* parses prefix and insures the next token is valid */
static ast_node_t*
parse_prefix(frontend_t *f)
{
	ast_node_t *node = NULL;
	lex_token_t token = f->lex.curr_token;

	/* remeber type to avoid doing lookup later */
	type_info_t type = NULL_TYPE_INFO;

	/* @todo: type checking */

	/* @refactor: repeating code */
	switch (token.type) {

		/* unary plus */
		case TOK_PLUS:

			token = lex_next_token(f);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(f, &token);
				type = ast_get_type_info(f, node);
			} else if (is_rvalue(token.type)) {
				node = make_rvalue_node(f, &token);
				type = node->literal.type;
			} else {
				syntax_error(f->lex.last_token.err_loc,
						"unexpected operand for unary operator");
			}

			return make_unary_node(f, AST_UNARY_PLUS, node, type);
	

		/* unary minus */
		case TOK_MINUS:

			token = lex_next_token(f);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(f, &token);
				type = ast_get_type_info(f, node);
			} else if (is_rvalue(token.type)) {
				node = make_rvalue_node(f, &token);
				type = node->literal.type;
			} else {
				syntax_error(f->lex.last_token.err_loc,
						"unexpected operand for unary operator");
			}

			return make_unary_node(f, AST_UNARY_MINUS, node, type);


		/* unary not*/
		case TOK_NOT:

			token = lex_next_token(f);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(f, &token);
				type = ast_get_type_info(f, node);
			} else if (is_rvalue(token.type)) {
				node = make_rvalue_node(f, &token);
				type = node->literal.type;
			} else {
				syntax_error(f->lex.last_token.err_loc,
						"unexpected operand for unary operator");
			}

			return make_unary_node(f, AST_UNARY_NOT, node, type);

		/* bitwise not */
		case TOK_NOT_BIT:

			token = lex_next_token(f);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(f, &token);
				type = ast_get_type_info(f, node);
			} else if (is_rvalue(token.type)) {
				node = make_rvalue_node(f, &token);
				type = node->literal.type;
			} else {
				syntax_error(f->lex.last_token.err_loc,
						"unexpected operand for unary operator");
			}

			return make_unary_node(f, AST_BIT_NOT, node, type);
		

		/* pre increment */
		case TOK_INCREASE:

			token = lex_next_token(f);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(f, &token);
				type = ast_get_type_info(f, node);
			} else if (is_rvalue(token.type)) {
				syntax_error(f->lex.last_token.err_loc,
						"r-value is not assignable");
			} else {
				syntax_error(f->lex.last_token.err_loc, "prefix error");
			}

			return make_unary_node(f, AST_PRE_INCREMENT, node, type);


		/* pre decrement */
		case TOK_DECREASE:

			token = lex_next_token(f);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(f, &token);
				type = ast_get_type_info(f, node);
			} else if (is_rvalue(token.type)) {
				syntax_error(f->lex.last_token.err_loc,
						"r-value is not assignable");
			} else {
				syntax_error(f->lex.last_token.err_loc, "prefix error");
			}
			
			return make_unary_node(f, AST_PRE_DECREMENT, node, type);


		/* address of */
		case TOK_AMPERSAND:

			/* skip '~' */
			token = lex_next_token(f);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(f, &token);
				type = ast_get_type_info(f, node);
				/* always on more level of indirection */
				++type.indirection;

			} else if (is_rvalue(token.type)) {
				syntax_error(f->lex.last_token.err_loc,
						"can not take address of rvalue");
			} else {
				syntax_error(f->lex.last_token.err_loc,
						"expected expression");
			}

			return make_unary_node(f, AST_ADDRESS, node, type);

		/* derefrence */
		case TOK_STAR:

			/* skip '*' */
			token = lex_next_token(f);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(f, &token);
				type = ast_get_type_info(f, node);

				/* make sure it's a pointer */
				if (type.indirection == 0) {
					syntax_error(f->lex.last_token.err_loc,
						"can not dereference non pointer");
				}

				/* set one level lo*/
				--type.indirection;

			} else if (is_rvalue(token.type)) {
				syntax_error(f->lex.last_token.err_loc,
						"can not dereference rvalue");
			} else {
				syntax_error(f->lex.last_token.err_loc,
						"expected expression");
			}

			return make_unary_node(f, AST_DEREF, node, type);

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
is_right_associavity(ast_type_t type)
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
expr_type_check(frontend_t *f, ast_node_t **left,
		   ast_node_t **right)
{
	type_compat_t compat;

	/* we fetch the compatibility of the righ and left nodes */
	compat = type_compat(ast_get_type_info(f, *left),
						 ast_get_type_info(f, *right));

	/* we lookup what to do depent in compat */
	switch (compat) {

		/* if the types a compatible we dont need to do anything */
		case TYPE_COMPAT_COMPAT:
			printf("type compat\n");
			return;

		/* we promote the left or the right */
		case TYPE_COMPAT_PROMOTE_LEFT:
			printf("type widen left\n");
			*left = make_ast_node(f, AST_PROMOTE, *left, NULL, NULL);
			break;

		case TYPE_COMPAT_PROMOTE_RIGHT:
			printf("type widen right\n");
			*right = make_ast_node(f, AST_PROMOTE, *right, NULL, NULL);
			break;

		/* if they are incompatible we throw a syntax error */
		case TYPE_COMPAT_INCOMPAT:
			syntax_error(f->lex.curr_token.err_loc,
					"type incompatible in expression");
			break;
	}
}

/* recursive pratt parser */
/* expect that the current token, is the first one of the expression */
static ast_node_t*
parse_expression(frontend_t *f, int32_t prev_prec,
				 lex_token_type_t terminator)
{
	lex_token_t token, next_token;
	ast_node_t *left = NULL, *right, *tmp;
	ast_type_t node_type;
	type_info_t expr_type;

	/* get value token */
	token = f->lex.curr_token;

	if (token.type == TOK_IDENTIFIER) {
		left = make_lvalue_node(f, &token);
	} else if (is_rvalue(token.type)) {
		left = make_rvalue_node(f, &token);
	} else if (token.type == TOK_PAREN_OPEN) {
		lex_next_token(f);
		left = parse_expression(f, 0, TOK_PAREN_CLOSED);
	} else {
		/* the only other valid option is a prefix */
		left = parse_prefix(f);

		if (!left) {
			syntax_error(token.err_loc, "failed parsing prefix");
		}
	}

	assert(left);

	/* the type of the expression is the type of
	 * the left node */
	expr_type = ast_get_type_info(f, left);

	/* get op token */
	next_token = lex_next_token(f);
	if (next_token.type == terminator) {
		return left;
	}

	node_type = op_tok_to_ast(next_token.type);
	if (!node_type) {
		syntax_error(err_loc(&f->lex), "expected expression");
	}

	/*
	 * if the prev token precedence is lower than the prev token
	 * we return (go up a level in the tree)
	*/
	while (prev_prec < ast_precedence[node_type] ||
		   (is_right_associavity(node_type) &&
		   prev_prec == ast_precedence[node_type])) {

		 /* parse the next part of expression */
		lex_next_token(f);
		right = parse_expression(f, ast_precedence[node_type], terminator);

		/* check type compat, and modify the type required */
		expr_type_check(f, &left, &right);	

		/* we must handle assignment differently */
		if (f->lex.curr_token.type == TOK_ASSIGN) {

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

		left = make_ast_node(f, node_type, left, NULL, right);

		/* we set the type of the expression */
		left->expr_type = expr_type;

		if (f->lex.curr_token.type == terminator) {
			return left;
		}

		node_type = op_tok_to_ast(f->lex.curr_token.type);
		if (!node_type) {
			syntax_error(f->lex.curr_token.err_loc,
					"expected expression");
		}
	}

	printf("exited expression with current token %s\n",
			lex_tok_debug_str(f->lex.curr_token.type));

	return left;
}

/* just used by the function below to call errors */
/* @todo: should perhaps have a loopup table with every type */
inline static const char*
tok_err_str(lex_token_type_t type)
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
assert_token(frontend_t *f, lex_token_type_t type)
{
	if (f->lex.curr_token.type != type) {
		syntax_error(f->lex.curr_token.err_loc, "expected '%s'",
				tok_err_str(type));
	}
}

/* parse single token, and throw error if different token is found */
inline static void
parse_token(frontend_t *f, lex_token_type_t type)
{
	assert_token(f, type);

	lex_next_token(f);
}

static ast_node_t*
parse_compound_statement(frontend_t *f);

/* asserts that we doesnt set a type prim more than once */
inline static void
set_type_prim(type_info_t *type, type_prim_t prim,
			  const err_location_t *err_loc)
{
	if (type->prim != TYPE_PRIM_NONE) {
		syntax_error(*err_loc, "duplicate type specifier");
	}

	type->prim = prim;
}

/* check that we doesnt set a type spec more than once,
 * we check for conflicts such as signed and unsigned when we are done */
inline static void
set_type_spec(type_info_t *type, type_spec_t spec,
			  const err_location_t *err_loc)
{
	if (type->spec & spec) {
		syntax_error(*err_loc, "duplicate type specifier");
	}

	type->spec |= spec;
}

static uint32_t
parse_pointer_indirection(frontend_t *f)
{
	lex_token_type_t type = f->lex.curr_token.type;
	uint32_t n = 0;

	while (type == TOK_STAR) {
		type = lex_next_token(f).type;
		++n;
	}

	return n;
}

/* parse a list of symbol specifier */
static type_info_t
parse_type(frontend_t *f)
{
	type_info_t type = NULL_TYPE_INFO;

	for (;;) {
		switch (f->lex.curr_token.type) {

			/* primitives */
			case TOK_KEY_INT:
				set_type_prim(&type, TYPE_PRIM_INT,
						&f->lex.curr_token.err_loc);
				break;

			case TOK_KEY_FLOAT:
				set_type_prim(&type, TYPE_PRIM_FLOAT,
						&f->lex.curr_token.err_loc);
				break;

			case TOK_KEY_CHAR:
				set_type_prim(&type, TYPE_PRIM_CHAR,
						&f->lex.curr_token.err_loc);
				break;

			case TOK_KEY_DOUBLE:
				set_type_prim(&type, TYPE_PRIM_DOUBLE,
						&f->lex.curr_token.err_loc);
				break;

			/* type specifiers */
			case TOK_KEY_CONST:
				set_type_spec(&type, TYPE_SPEC_CONST,
						&f->lex.curr_token.err_loc);
				break;

			case TOK_KEY_STATIC:
				set_type_spec(&type, TYPE_SPEC_STATIC,
						&f->lex.curr_token.err_loc);
				break;

			case TOK_KEY_UNSIGNED:
				set_type_spec(&type, TYPE_SPEC_UNSIGNED,
						&f->lex.curr_token.err_loc);
				break;

			case TOK_KEY_SIGNED:
				set_type_spec(&type, TYPE_SPEC_SIGNED,
						&f->lex.curr_token.err_loc);
				break;

			case TOK_KEY_SHORT:
				set_type_spec(&type, TYPE_SPEC_SHORT,
						&f->lex.curr_token.err_loc);
				break;

			case TOK_KEY_LONG:
				set_type_spec(&type, TYPE_SPEC_LONG,
						&f->lex.curr_token.err_loc);
				break;

			case TOK_KEY_REGISTER:
				set_type_spec(&type, TYPE_SPEC_REGISTER,
						&f->lex.curr_token.err_loc);
				break;

			case TOK_KEY_VOLATILE:
				set_type_spec(&type, TYPE_SPEC_VOLATILE,
						&f->lex.curr_token.err_loc);
				break;

			case TOK_KEY_EXTERN:
				set_type_spec(&type, TYPE_SPEC_EXTERN,
						&f->lex.curr_token.err_loc);
				break;

			default:
				type.indirection = parse_pointer_indirection(f);
				return type;
		}

		lex_next_token(f);
	}
}

/* expect to be on '(' */
static vec_sym_param_t
parse_parameter_list(frontend_t *f)
{
	sym_param_t param;	
	vec_sym_param_t vec = vec_sym_param_t_create(4);

	lex_next_token(f);

	for (;;) {
		param.type = parse_type(f);
		param.err_loc = f->lex.curr_token.err_loc;

		switch (f->lex.curr_token.type) {

			case TOK_IDENTIFIER:
				param.hash = f->lex.curr_token.hash;
				lex_next_token(f);
				break;

			/* anon identifier */
			case TOK_COMMA:
				/* anonomous identifiers are allowed for function
				 * declarations, but not function definitions,
				 * we just set a null hash, and check for that later */	
				param.hash = SYM_NULL_HASH;
				break;

			case TOK_PAREN_CLOSED:
				param.hash = SYM_NULL_HASH;
				lex_next_token(f);
				return vec;

			default:
				// printf("token type: %s\n", lex_tok_debug_str(f->lex.curr_token.type));
				syntax_error(f->lex.curr_token.err_loc, "parameter unexpectet");
				break;
		}

		type_check_validity(&param.type, &f->lex.last_token.err_loc);
		vec_sym_param_t_push(&vec, param);

		switch (f->lex.curr_token.type) {
			case TOK_COMMA:
				lex_next_token(f);	
				break;

			case TOK_PAREN_CLOSED:
				lex_next_token(f);
				return vec;

			default:
				printf("token type: %s\n", lex_tok_debug_str(f->lex.curr_token.type));
				syntax_error(f->lex.curr_token.err_loc, "parameter unexpectet");
				break;
		}
	}
}

static ast_node_t*
parse_function(frontend_t *f, sym_global_t global, sym_hash_t hash)
{
	global.function.params = parse_parameter_list(f);

	/* functiom definition */
	if (f->lex.curr_token.type == TOK_BRACE_OPEN) {

		sym_check_for_anon_params(&global.function.params);
		
		sym_define_global(&f->sym_table, global, hash, &f->lex.curr_token.err_loc);

		/* we shouldent have to check this, since there will be
		 * a conflict if when we add the params to scope */
		/* sym_check_for_duplicate_params(&global.function.params); */

		/* enter function scope */
		sym_push_scope(&f->sym_table);

		sym_add_params_to_scope(&f->sym_table, &global.function.params);
	
		/* @todo: implement parse_function_body */
		return parse_compound_statement(f);
	}
	/* function declaration */
	else {
		sym_check_for_duplicate_params(&global.function.params);

		sym_declare_global(&f->sym_table, global, hash, &f->lex.curr_token.err_loc);

		parse_token(f, TOK_SEMIKOLON);
		return NULL;
	}
}

static ast_node_t*
parse_global_declaration(frontend_t *f)
{
	sym_global_t global;
	sym_hash_t hash;

	global.type = parse_type(f);

	type_check_validity(&global.type, &f->lex.curr_token.err_loc);

	hash = f->lex.curr_token.hash;

	parse_token(f, TOK_IDENTIFIER);

	switch (f->lex.curr_token.type) {

		/* def of global var */
		case TOK_ASSIGN:

			/* @todo: add some kind of compile time expression parser */
			parse_expression(f, 0, TOK_SEMIKOLON);

			global.val.val_int = 0;
			global.kind = SYM_GLOBAL_KIND_VARIABLE;

			sym_define_global(&f->sym_table, global, hash, &f->lex.curr_token.err_loc);

			lex_next_token(f);			
			return NULL;

		/* decl of global var */
		case TOK_SEMIKOLON:
			global.kind = SYM_GLOBAL_KIND_VARIABLE;
			sym_declare_global(&f->sym_table, global, hash, &f->lex.curr_token.err_loc);

			lex_next_token(f);
			return NULL;

		/* def or decl of function */
		case TOK_PAREN_OPEN:
			global.kind = SYM_GLOBAL_KIND_FUNCTION;
			return parse_function(f, global, hash);

		default:
			syntax_error(f->lex.curr_token.err_loc, "missing semikolon");
			return NULL;
	}
}

/* called when we get a type specifier token */
static ast_node_t*
parse_local_definition(frontend_t *f)
{
	sym_id_t id;
	sym_hash_t hash;
	sym_local_t local;
	ast_node_t *right, *left;

	/* loops through type specifiers */
	local.type = parse_type(f);
	local.kind = SYM_LOCAL_KIND_VARIABLE;

	/* check for conflicts */
	type_check_validity(&local.type, &f->lex.curr_token.err_loc);	

	hash = f->lex.curr_token.hash;

	/* the next token after specifiers must be an identifier */
	parse_token(f, TOK_IDENTIFIER);

	id = sym_define_local(&f->sym_table, local, hash, &f->lex.curr_token.err_loc);

	/* check for inline assignment and functions and such */
	switch (f->lex.curr_token.type) {

		/* inline assignment */
		case TOK_ASSIGN:

			lex_next_token(f);

			/* make an identifier ast node */
			right = make_ast_node(f, AST_IDENTIFIER, NULL, NULL, NULL);
			right->sym_id = id;

			/* parse the expression */
			left = parse_expression(f, 0, TOK_SEMIKOLON);

			/* skip semikolon */
			lex_next_token(f);

			/* glue together expression */
			left = make_ast_node(f, AST_ASSIGN, left, NULL, right);

			return left;

		case TOK_SEMIKOLON:
			lex_next_token(f);
			
			return NULL;
			

		/* undeclared  */	
		default:
			syntax_error(f->lex.curr_token.err_loc, "expected semikolon");
			return NULL;
	}
}

static ast_node_t*
parse_else_statement(frontend_t *f)
{
	lex_next_token(f);
	return parse_statement(f);
}

static ast_node_t*
parse_if_statement(frontend_t *f)
{
	ast_node_t *condition_ast, *true_ast, *false_ast;

	/* skip 'if' token */
	lex_next_token(f);

	/* skip opening '(' */
	parse_token(f, TOK_PAREN_OPEN);

	/* parse conditional expression */
	condition_ast = parse_expression(f, 0, TOK_PAREN_CLOSED);

	/* skip closing ')' */
	lex_next_token(f);

	/* parse the body */
	true_ast = parse_statement(f);

	if (f->lex.curr_token.type == TOK_KEY_ELSE) {
		lex_next_token(f);
		false_ast = parse_else_statement(f);
	}
	else {
		false_ast = NULL;
	}

	return make_ast_node(f, AST_IF, condition_ast, true_ast, false_ast);
}

static ast_node_t*
parse_while_loop(frontend_t *f)
{
	ast_node_t *condition_ast, *body_ast;

	/* skip 'while' token */	
	lex_next_token(f);

	/* skip the '(' */
	parse_token(f, TOK_PAREN_OPEN);

	/* parse the conditional */
	condition_ast = parse_expression(f, 0, TOK_PAREN_CLOSED);

	/* skip closing ')' */
	lex_next_token(f);

	/* parse the body */
	body_ast = parse_statement(f);

	/* glue body and conditional together with a while ast */
	return make_ast_node(f, AST_WHILE, condition_ast, NULL, body_ast);

}

static ast_node_t*
parse_for_loop(frontend_t *f)
{
	ast_node_t *tree, *body, *cond, *pre_op, *post_op;

	/* skip opening 'for' token */
	lex_next_token(f);

	/* skip the '(' */
	parse_token(f, TOK_PAREN_OPEN);

	/* parse preop expression */
	pre_op = parse_expression(f, 0, TOK_SEMIKOLON);
	lex_next_token(f);

	/* parse conditional statement */
	cond = parse_expression(f, 0, TOK_SEMIKOLON);
	lex_next_token(f);

	/* parse post op */	
	post_op = parse_expression(f, 0, TOK_PAREN_CLOSED);
	lex_next_token(f);

	/* parse statement */
	body = parse_statement(f);

	/* glue the body and post op so we does the postop after the body */	
	tree = make_ast_node(f, AST_NOP, body, NULL, post_op);

	/* we make a while loop with the body and cond */
	tree = make_ast_node(f, AST_WHILE, cond, NULL, tree);

	/* glue the preop and while loop so we does the preop before the loop */
	tree = make_ast_node(f, AST_NOP, pre_op, NULL, tree);

	return tree;
}

/* creates an ast tree from a single statement, which can include compound statements */
/* expect the current token to be the first of the statement */
ast_node_t*
parse_statement(frontend_t *f)
{
	lex_token_t token = f->lex.curr_token;
	ast_node_t *tree;

	/* we sholdnt have to have to enter a new scope
	 * since you cant declare a new variabel, and use it in a single
	 * statement, probably?? */

	switch (token.type) {
		case TOK_KEY_IF:
			return parse_if_statement(f);

		case TOK_KEY_FOR:
			return parse_for_loop(f);

		case TOK_KEY_WHILE:
			return parse_while_loop(f);

		case TOK_KEY_ELSE:
			syntax_error(token.err_loc, "no maching if statement");
			return NULL;

		case TOK_BRACE_OPEN:
			/* we enter a new scope */
			sym_push_scope(&f->sym_table);

			return parse_compound_statement(f);

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
			return parse_local_definition(f);

		default:
			tree = parse_expression(f, 0, TOK_SEMIKOLON);

			/* skip semikolon */
			lex_next_token(f);

			return tree;
	}
}

/* loops through all statements in a compound statement and creates an ast tree */
static ast_node_t*
parse_compound_statement(frontend_t *f)
{
	ast_node_t *left = NULL, *tree = NULL;

	/* skip opening '{', shouldnt have to check */
	lex_next_token(f);

	/* parse all statements until we get a closed brace */
	for (;;) {
		if (f->lex.curr_token.type == TOK_BRACE_CLOSED) {
			/* skip closing '}' */
			lex_next_token(f);

			/* leave current scope */
			sym_pop_scope(&f->sym_table);

			return left;
		}

		/* parse_statement mat call parse_comound_statement,
		 * so it is called recursively in nested comound statements */
		tree = parse_statement(f);
		
		if (tree) {
			/* if its the first statement */
			if (!left) {
				left = tree;
			} else {
				/* glue together with prev statements */
				left = make_ast_node(f, AST_NOP, left, NULL, tree);
			}
		}
	}
}

static ast_node_t*
parse_function_body(frontend_t *f)
{

}
/* ===================================================================== */
/*  ============================ PUBLIC =============================== */
/* ===================================================================== */

ast_node_t*
ast_parse(frontend_t *f)
{
	lex_token_t token = f->lex.curr_token;
	ast_node_t* function;

	for (;;) {

		switch (token.type) {
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
				function = parse_global_declaration(f);

				if (function) {
					return function;
				}

				break;

			case TOK_EOF:
				return NULL;

			default:
				printf("parsing token: %s\n", lex_tok_debug_str(token.type));
				syntax_error(f->lex.curr_token.err_loc, "failure parsing");
		}
	}
	
}

/* @debug: make this easier to read in the console */
void
ast_print_tree(ast_node_t *node, uint32_t level)
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
ast_print_tree_postorder(ast_node_t *node)
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
