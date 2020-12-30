#include "f_parser.h"
#include "f_lexer.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

/*
 * We parse the token inputstream, and parse them into a more abstract
 * and lower level abstract syntax tree,
 * which make sure the syntax is correct, expression happen in the right
 * order and prepares the syntax for optimization and code generation.
 */

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
ast_get_type_info(parser_t *parser, ast_node_t *node)
{
	switch (node->type) {

		case AST_LITERAL:
			return type_from_literal(node->literal);

		case AST_IDENTIFIER:
			return ((sym_global_t*)sym_get_entry(parser->sym_table, node->sym_id))->type;

		/* we just assume its some other node as part of and expression,
		 * or an unary node, in which case we also save
		 * the type of what its pointing at in expr_type*/
		default:
			return node->expr_type;
	}
}


/* allocate and create ast node */
inline static ast_node_t*
make_ast_node(parser_t *parser, ast_type_t type,
			  ast_node_t *left, ast_node_t *center,
			  ast_node_t *right)
{
	ast_node_t *node	= mem_pool_alloc(&parser->pool, sizeof(ast_node_t));
	node->left			= left;
	node->center		= center;
	node->right			= right;
	node->type			= type;

	return node;
}

/* create an */
inline static ast_node_t*
make_rvalue_node(parser_t *parser, const lex_token_t *token)
{
	ast_node_t *node;

	switch (token->type) {

		case TOK_LITERAL:
			node = make_ast_node(parser, AST_LITERAL, NULL, NULL, NULL);
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
make_lvalue_node(parser_t *parser, const lex_token_t *token)
{
	sym_id_t id;
	ast_node_t *node;

	id = sym_find_id(parser->sym_table, token->hash);

	/* sym will always be defined before we use it here */
	if (id == SYM_ID_NULL) {
		syntax_error(token->err_loc, "use of undeclared identifier");
	}

	node = make_ast_node(parser, AST_IDENTIFIER, NULL, NULL, NULL);
	node->sym_id = id;

	return node;
}

/* helper function to create unary nodes */
inline static ast_node_t*
make_unary_node(parser_t *parser, ast_type_t type,
				ast_node_t *node, type_info_t type_info)
{
	ast_node_t *unary_node;

	unary_node = make_ast_node(parser, type, node, NULL, NULL);

	/* the type may depent on the type of prefix */
	unary_node->expr_type = type_info;

	return unary_node;
}

/* parse function call, curr token must be on identifier */
static ast_node_t*
parse_function_call(parser_t *parser)
{
	sym_global_t *func;

	/* get current token */
	lex_token_t token = parser->lexer->curr_token;

	/* find the symbol */
	int32_t id = sym_find_id(parser->sym_table, token.hash);

	/* make sure that it's declared */
	if (id == SYM_ID_NULL) {
		syntax_error(token.err_loc, "undefined function");
	}

	/* get entry */
	func = sym_get_global(parser->sym_table, id);

	/* make sure its a function */
	if (func->kind != SYM_GLOBAL_KIND_FUNCTION) {
		syntax_error(token.err_loc, "cannot call variabel");
	}

	/* skip identifier */
	f_next_token(parser->lexer);

	/* skip '(', we should not have to check */
	f_next_token(parser->lexer);

	/*  */

}

/* peek at next token, and determine if it's a postfix, and
 * if it is we generate the ast tree, and skip the postfix,
 * otherwise return NULL */
static ast_node_t*
parse_postfix(parser_t *parser)
{
	/* peek at next token */
	lex_token_t token = parser->lexer->next_token;

	switch (token.type) {

		/* function call */
		case TOK_PAREN_OPEN:

			return parse_function_call(parser);

		/* post increment */
		case TOK_INCREASE:

			f_next_token(parser->lexer);

			return make_ast_node(parser, AST_POST_INCREMENT, NULL, NULL, NULL);

		/* post decrement */
		case TOK_DECREASE:

			f_next_token(parser->lexer);

			return make_ast_node(parser, AST_POST_DECREMENT, NULL, NULL, NULL);

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
parse_prefix(parser_t *parser)
{
	ast_node_t *node = NULL;
	lex_token_t token = parser->lexer->curr_token;

	/* remeber type to avoid doing lookup later */
	type_info_t type = NULL_TYPE_INFO;

	/* @todo: type checking */

	/* @refactor: repeating code */
	switch (token.type) {

		/* unary plus */
		case TOK_PLUS:

			token = f_next_token(parser->lexer);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(parser, &token);
				type = ast_get_type_info(parser, node);
			} else if (is_rvalue(token.type)) {
				node = make_rvalue_node(parser, &token);
				type = type_from_literal(node->literal);
			} else {
				syntax_error(parser->lexer->last_token.err_loc,
						"unexpected operand for unary operator");
			}

			return make_unary_node(parser, AST_UNARY_PLUS, node, type);


		/* unary minus */
		case TOK_MINUS:

			token = f_next_token(parser->lexer);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(parser, &token);
				type = ast_get_type_info(parser, node);
			} else if (is_rvalue(token.type)) {
				node = make_rvalue_node(parser, &token);
				type = type_from_literal(node->literal);
			} else {
				syntax_error(parser->lexer->last_token.err_loc,
						"unexpected operand for unary operator");
			}

			return make_unary_node(parser, AST_UNARY_MINUS, node, type);


		/* unary not*/
		case TOK_NOT:

			token = f_next_token(parser->lexer);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(parser, &token);
				type = ast_get_type_info(parser, node);
			} else if (is_rvalue(token.type)) {
				node = make_rvalue_node(parser, &token);
				type = type_from_literal(node->literal);
			} else {
				syntax_error(parser->lexer->last_token.err_loc,
						"unexpected operand for unary operator");
			}

			return make_unary_node(parser, AST_UNARY_NOT, node, type);

		/* bitwise not */
		case TOK_NOT_BIT:

			token = f_next_token(parser->lexer);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(parser, &token);
				type = ast_get_type_info(parser, node);
			} else if (is_rvalue(token.type)) {
				node = make_rvalue_node(parser, &token);
				type = type_from_literal(node->literal);
			} else {
				syntax_error(parser->lexer->last_token.err_loc,
						"unexpected operand for unary operator");
			}

			return make_unary_node(parser, AST_BIT_NOT, node, type);


		/* pre increment */
		case TOK_INCREASE:

			token = f_next_token(parser->lexer);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(parser, &token);
				type = ast_get_type_info(parser, node);
			} else if (is_rvalue(token.type)) {
				syntax_error(parser->lexer->last_token.err_loc,
						"r-value is not assignable");
			} else {
				syntax_error(parser->lexer->last_token.err_loc, "prefix error");
			}

			return make_unary_node(parser, AST_PRE_INCREMENT, node, type);


		/* pre decrement */
		case TOK_DECREASE:

			token = f_next_token(parser->lexer);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(parser, &token);
				type = ast_get_type_info(parser, node);
			} else if (is_rvalue(token.type)) {
				syntax_error(parser->lexer->last_token.err_loc,
						"r-value is not assignable");
			} else {
				syntax_error(parser->lexer->last_token.err_loc, "prefix error");
			}

			return make_unary_node(parser, AST_PRE_DECREMENT, node, type);


		/* address of */
		case TOK_AMPERSAND:

			/* skip '~' */
			token = f_next_token(parser->lexer);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(parser, &token);
				type = ast_get_type_info(parser, node);
				/* always on more level of indirection */
				++type.indirection;

			} else if (is_rvalue(token.type)) {
				syntax_error(parser->lexer->last_token.err_loc,
						"can not take address of rvalue");
			} else {
				syntax_error(parser->lexer->last_token.err_loc,
						"expected expression");
			}

			return make_unary_node(parser, AST_ADDRESS, node, type);

		/* derefrence */
		case TOK_STAR:

			/* skip '*' */
			token = f_next_token(parser->lexer);

			if (token.type == TOK_IDENTIFIER) {
				node = make_lvalue_node(parser, &token);
				type = ast_get_type_info(parser, node);

				/* make sure it's a pointer */
				if (type.indirection == 0) {
					syntax_error(parser->lexer->last_token.err_loc,
						"can not dereference non pointer");
				}

				/* set one level lo*/
				--type.indirection;

			} else if (is_rvalue(token.type)) {
				syntax_error(parser->lexer->last_token.err_loc,
						"can not dereference rvalue");
			} else {
				syntax_error(parser->lexer->last_token.err_loc,
						"expected expression");
			}

			return make_unary_node(parser, AST_DEREF, node, type);

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
expr_type_check(parser_t *parser, ast_node_t **left,
		   ast_node_t **right)
{
	type_compat_t compat;

	/* we fetch the compatibility of the righ and left nodes */
	compat = type_compat(ast_get_type_info(parser, *left),
						 ast_get_type_info(parser, *right));

	/* we lookup what to do depent in compat */
	switch (compat) {

		/* if the types a compatible we dont need to do anything */
		case TYPE_COMPAT_COMPAT:
			printf("type compat\n");
			return;

		/* we promote the left or the right */
		case TYPE_COMPAT_PROMOTE_LEFT:
			printf("type widen left\n");
			*left = make_ast_node(parser, AST_PROMOTE, *left, NULL, NULL);
			break;

		case TYPE_COMPAT_PROMOTE_RIGHT:
			printf("type widen right\n");
			*right = make_ast_node(parser, AST_PROMOTE, *right, NULL, NULL);
			break;

		/* if they are incompatible we throw a syntax error */
		case TYPE_COMPAT_INCOMPAT:
			syntax_error(parser->lexer->curr_token.err_loc,
					"type incompatible in expression");
			break;
	}
}

/* recursive pratt parser */
/* expect that the current token, is the first one of the expression */
static ast_node_t*
parse_expression(parser_t *parser, int32_t prev_prec,
				 lex_token_type_t terminator)
{
	lex_token_t token, next_token;
	ast_node_t *left = NULL, *right, *tmp;
	ast_type_t node_type;
	type_info_t expr_type;

	/* get value token */
	token = parser->lexer->curr_token;

	if (token.type == TOK_IDENTIFIER) {
		left = make_lvalue_node(parser, &token);
	} else if (is_rvalue(token.type)) {
		left = make_rvalue_node(parser, &token);
	} else if (token.type == TOK_PAREN_OPEN) {
		f_next_token(parser->lexer);
		left = parse_expression(parser, 0, TOK_PAREN_CLOSED);
	} else {
		/* the only other valid option is a prefix */
		left = parse_prefix(parser);

		if (!left) {
			syntax_error(token.err_loc, "failed parsing prefix");
		}
	}

	assert(left);

	/* the type of the expression is the type of
	 * the left node */
	expr_type = ast_get_type_info(parser, left);

	/* get op token */
	next_token = f_next_token(parser->lexer);
	if (next_token.type == terminator) {
		return left;
	}

	node_type = op_tok_to_ast(next_token.type);
	if (!node_type) {
		syntax_error(err_loc(parser->lexer), "expected expression");
	}

	/*
	 * if the prev token precedence is lower than the prev token
	 * we return (go up a level in the tree)
	*/
	while (prev_prec < ast_precedence[node_type] ||
		   (is_right_associavity(node_type) &&
		   prev_prec == ast_precedence[node_type])) {

		 /* parse the next part of expression */
		f_next_token(parser->lexer);
		right = parse_expression(parser, ast_precedence[node_type], terminator);

		/* check type compat, and modify the type required */
		expr_type_check(parser, &left, &right);

		/* we must handle assignment differently */
		if (parser->lexer->curr_token.type == TOK_ASSIGN) {

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

		left = make_ast_node(parser, node_type, left, NULL, right);

		/* we set the type of the expression */
		left->expr_type = expr_type;

		if (parser->lexer->curr_token.type == terminator) {
			return left;
		}

		node_type = op_tok_to_ast(parser->lexer->curr_token.type);
		if (!node_type) {
			syntax_error(parser->lexer->curr_token.err_loc,
					"expected expression");
		}
	}

	printf("exited expression with current token %s\n",
			lex_tok_debug_str(parser->lexer->curr_token.type));

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
assert_token(parser_t *parser, lex_token_type_t type)
{
	if (parser->lexer->curr_token.type != type) {
		syntax_error(parser->lexer->curr_token.err_loc, "expected '%s'",
				tok_err_str(type));
	}
}

/* parse single token, and throw error if different token is found */
inline static void
parse_token(parser_t *parser, lex_token_type_t type)
{
	assert_token(parser, type);

	f_next_token(parser->lexer);
}

static ast_node_t*
parse_compound_statement(parser_t *parser);

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
parse_pointer_indirection(parser_t *parser)
{
	lex_token_type_t type = parser->lexer->curr_token.type;
	uint32_t n = 0;

	while (type == TOK_STAR) {
		type = f_next_token(parser->lexer).type;
		++n;
	}

	return n;
}

/* parse a list of symbol specifier */
static type_info_t
parse_type(parser_t *parser)
{
	type_info_t type = NULL_TYPE_INFO;

	for (;;) {
		switch (parser->lexer->curr_token.type) {

			/* primitives */
			case TOK_KEY_INT:
				set_type_prim(&type, TYPE_PRIM_INT,
						&parser->lexer->curr_token.err_loc);
				break;

			case TOK_KEY_FLOAT:
				set_type_prim(&type, TYPE_PRIM_FLOAT,
						&parser->lexer->curr_token.err_loc);
				break;

			case TOK_KEY_CHAR:
				set_type_prim(&type, TYPE_PRIM_CHAR,
						&parser->lexer->curr_token.err_loc);
				break;

			case TOK_KEY_DOUBLE:
				set_type_prim(&type, TYPE_PRIM_DOUBLE,
						&parser->lexer->curr_token.err_loc);
				break;

			/* type specifiers */
			case TOK_KEY_CONST:
				set_type_spec(&type, TYPE_SPEC_CONST,
						&parser->lexer->curr_token.err_loc);
				break;

			case TOK_KEY_STATIC:
				set_type_spec(&type, TYPE_SPEC_STATIC,
						&parser->lexer->curr_token.err_loc);
				break;

			case TOK_KEY_UNSIGNED:
				set_type_spec(&type, TYPE_SPEC_UNSIGNED,
						&parser->lexer->curr_token.err_loc);
				break;

			case TOK_KEY_SIGNED:
				set_type_spec(&type, TYPE_SPEC_SIGNED,
						&parser->lexer->curr_token.err_loc);
				break;

			case TOK_KEY_SHORT:
				set_type_spec(&type, TYPE_SPEC_SHORT,
						&parser->lexer->curr_token.err_loc);
				break;

			case TOK_KEY_LONG:
				set_type_spec(&type, TYPE_SPEC_LONG,
						&parser->lexer->curr_token.err_loc);
				break;

			case TOK_KEY_REGISTER:
				set_type_spec(&type, TYPE_SPEC_REGISTER,
						&parser->lexer->curr_token.err_loc);
				break;

			case TOK_KEY_VOLATILE:
				set_type_spec(&type, TYPE_SPEC_VOLATILE,
						&parser->lexer->curr_token.err_loc);
				break;

			case TOK_KEY_EXTERN:
				set_type_spec(&type, TYPE_SPEC_EXTERN,
						&parser->lexer->curr_token.err_loc);
				break;

			default:
				type.indirection = parse_pointer_indirection(parser);
				return type;
		}

		f_next_token(parser->lexer);
	}
}

/* expect to be on '(' */
static vec_sym_param_t
parse_parameter_list(parser_t *parser)
{
	sym_param_t param;
	vec_sym_param_t vec = vec_sym_param_t_create(4);

	f_next_token(parser->lexer);

	for (;;) {
		param.type = parse_type(parser);
		param.err_loc = parser->lexer->curr_token.err_loc;

		switch (parser->lexer->curr_token.type) {

			case TOK_IDENTIFIER:
				param.hash = parser->lexer->curr_token.hash;
				f_next_token(parser->lexer);
				break;

			/* anon identifier */
			case TOK_COMMA:
			case TOK_PAREN_CLOSED:
				/* anonomous identifiers are allowed for function
				 * declarations, but not function definitions,
				 * we just set a null hash, and check for that later */
				param.hash = SYM_NULL_HASH;
				break;

			default:
				// printf("token type: %s\n", lex_tok_debug_str(parser->lexer->curr_token.type));
				syntax_error(parser->lexer->curr_token.err_loc, "parameter unexpectet");
				break;
		}

		type_check_validity(&param.type, &parser->lexer->last_token.err_loc);
		vec_sym_param_t_push(&vec, param);

		switch (parser->lexer->curr_token.type) {
			case TOK_COMMA:
				f_next_token(parser->lexer);
				break;

			case TOK_PAREN_CLOSED:
				f_next_token(parser->lexer);
				return vec;

			default:
				printf("token type: %s\n", lex_tok_debug_str(parser->lexer->curr_token.type));
				syntax_error(parser->lexer->curr_token.err_loc, "parameter unexpectet");
				break;
		}
	}
}

static ast_node_t*
parse_function(parser_t *parser, sym_global_t global, sym_hash_t hash)
{
	global.function.params = parse_parameter_list(parser);

	/* functiom definition */
	if (parser->lexer->curr_token.type == TOK_BRACE_OPEN) {

		sym_check_for_anon_params(&global.function.params);

		sym_define_global(parser->sym_table, global, hash, &parser->lexer->curr_token.err_loc);

		/* we shouldent have to check this, since there will be
		 * a conflict if when we add the params to scope */
		/* sym_check_for_duplicate_params(&global.function.params); */

		/* enter function scope */
		sym_push_scope(parser->sym_table);

		sym_add_params_to_scope(parser->sym_table, &global.function.params);

		/* @todo: implement parse_function_body */
		return parse_compound_statement(parser);
	}
	/* function declaration */
	else {
		sym_check_for_duplicate_params(&global.function.params);

		sym_declare_global(parser->sym_table, global, hash, &parser->lexer->curr_token.err_loc);

		parse_token(parser, TOK_SEMIKOLON);
		return NULL;
	}
}

static ast_node_t*
parse_global_declaration(parser_t *parser)
{
	sym_global_t global;
	sym_hash_t hash;

	global.type = parse_type(parser);

	type_check_validity(&global.type, &parser->lexer->curr_token.err_loc);

	hash = parser->lexer->curr_token.hash;

	parse_token(parser, TOK_IDENTIFIER);

	switch (parser->lexer->curr_token.type) {

		/* def of global var */
		case TOK_ASSIGN:

			/* @todo: add some kind of compile time expression parser */
			parse_expression(parser, 0, TOK_SEMIKOLON);

			global.val._int = 0;
			global.kind = SYM_GLOBAL_KIND_VARIABLE;

			sym_define_global(parser->sym_table, global, hash, &parser->lexer->curr_token.err_loc);

			f_next_token(parser->lexer);
			return NULL;

		/* decl of global var */
		case TOK_SEMIKOLON:
			global.kind = SYM_GLOBAL_KIND_VARIABLE;
			sym_declare_global(parser->sym_table, global, hash, &parser->lexer->curr_token.err_loc);

			f_next_token(parser->lexer);
			return NULL;

		/* def or decl of function */
		case TOK_PAREN_OPEN:
			global.kind = SYM_GLOBAL_KIND_FUNCTION;
			return parse_function(parser, global, hash);

		default:
			syntax_error(parser->lexer->curr_token.err_loc, "missing semikolon");
			return NULL;
	}
}

/* called when we get a type specifier token */
static ast_node_t*
parse_local_definition(parser_t *parser)
{
	sym_id_t id;
	sym_hash_t hash;
	sym_local_t local;
	ast_node_t *right, *left;

	/* loops through type specifiers */
	local.type = parse_type(parser);
	local.kind = SYM_LOCAL_KIND_VARIABLE;

	/* check for conflicts */
	type_check_validity(&local.type, &parser->lexer->curr_token.err_loc);

	hash = parser->lexer->curr_token.hash;

	/* the next token after specifiers must be an identifier */
	parse_token(parser, TOK_IDENTIFIER);

	id = sym_define_local(parser->sym_table, local, hash, &parser->lexer->curr_token.err_loc);

	/* check for inline assignment and functions and such */
	switch (parser->lexer->curr_token.type) {

		/* inline assignment */
		case TOK_ASSIGN:

			f_next_token(parser->lexer);

			/* make an identifier ast node */
			right = make_ast_node(parser, AST_IDENTIFIER, NULL, NULL, NULL);
			right->sym_id = id;

			/* parse the expression */
			left = parse_expression(parser, 0, TOK_SEMIKOLON);

			/* skip semikolon */
			f_next_token(parser->lexer);

			/* glue together expression */
			left = make_ast_node(parser, AST_ASSIGN, left, NULL, right);

			return left;

		case TOK_SEMIKOLON:
			f_next_token(parser->lexer);

			return NULL;


		/* undeclared  */
		default:
			syntax_error(parser->lexer->curr_token.err_loc, "expected semikolon");
			return NULL;
	}
}

static ast_node_t*
parse_else_statement(parser_t *parser)
{
	f_next_token(parser->lexer);
	return parse_statement(parser);
}

static ast_node_t*
parse_if_statement(parser_t *parser)
{
	ast_node_t *condition_ast, *true_ast, *false_ast;

	/* skip 'if' token */
	f_next_token(parser->lexer);

	/* skip opening '(' */
	parse_token(parser, TOK_PAREN_OPEN);

	/* parse conditional expression */
	condition_ast = parse_expression(parser, 0, TOK_PAREN_CLOSED);

	/* skip closing ')' */
	f_next_token(parser->lexer);

	/* parse the body */
	true_ast = parse_statement(parser);

	if (parser->lexer->curr_token.type == TOK_KEY_ELSE) {
		f_next_token(parser->lexer);
		false_ast = parse_else_statement(parser);
	}
	else {
		false_ast = NULL;
	}

	return make_ast_node(parser, AST_IF, condition_ast, true_ast, false_ast);
}

static ast_node_t*
parse_while_loop(parser_t *parser)
{
	ast_node_t *condition_ast, *body_ast;

	/* skip 'while' token */
	f_next_token(parser->lexer);

	/* skip the '(' */
	parse_token(parser, TOK_PAREN_OPEN);

	/* parse the conditional */
	condition_ast = parse_expression(parser, 0, TOK_PAREN_CLOSED);

	/* skip closing ')' */
	f_next_token(parser->lexer);

	/* parse the body */
	body_ast = parse_statement(parser);

	/* glue body and conditional together with a while ast */
	return make_ast_node(parser, AST_WHILE, condition_ast, NULL, body_ast);

}

static ast_node_t*
parse_for_loop(parser_t *parser)
{
	ast_node_t *tree, *body, *cond, *pre_op, *post_op;

	/* skip opening 'for' token */
	f_next_token(parser->lexer);

	/* skip the '(' */
	parse_token(parser, TOK_PAREN_OPEN);

	/* parse preop expression */
	pre_op = parse_expression(parser, 0, TOK_SEMIKOLON);
	f_next_token(parser->lexer);

	/* parse conditional statement */
	cond = parse_expression(parser, 0, TOK_SEMIKOLON);
	f_next_token(parser->lexer);

	/* parse post op */
	post_op = parse_expression(parser, 0, TOK_PAREN_CLOSED);
	f_next_token(parser->lexer);

	/* parse statement */
	body = parse_statement(parser);

	/* glue the body and post op so we does the postop after the body */
	tree = make_ast_node(parser, AST_NOP, body, NULL, post_op);

	/* we make a while loop with the body and cond */
	tree = make_ast_node(parser, AST_WHILE, cond, NULL, tree);

	/* glue the preop and while loop so we does the preop before the loop */
	tree = make_ast_node(parser, AST_NOP, pre_op, NULL, tree);

	return tree;
}

/* creates an ast tree from a single statement, which can include compound statements */
/* expect the current token to be the first of the statement */
ast_node_t*
parse_statement(parser_t *parser)
{
	lex_token_t token = parser->lexer->curr_token;
	ast_node_t *tree;

	/* we sholdnt have to have to enter a new scope
	 * since you cant declare a new variabel, and use it in a single
	 * statement, probably?? */

	switch (token.type) {
		case TOK_KEY_IF:
			return parse_if_statement(parser);

		case TOK_KEY_FOR:
			return parse_for_loop(parser);

		case TOK_KEY_WHILE:
			return parse_while_loop(parser);

		case TOK_KEY_ELSE:
			syntax_error(token.err_loc, "no maching if statement");
			return NULL;

		case TOK_BRACE_OPEN:
			/* we enter a new scope */
			sym_push_scope(parser->sym_table);

			return parse_compound_statement(parser);

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
			return parse_local_definition(parser);

		default:
			tree = parse_expression(parser, 0, TOK_SEMIKOLON);

			/* skip semikolon */
			f_next_token(parser->lexer);

			return tree;
	}
}

/* loops through all statements in a compound statement and creates an ast tree */
static ast_node_t*
parse_compound_statement(parser_t *parser)
{
	ast_node_t *left = NULL, *tree = NULL;

	/* skip opening '{', shouldnt have to check */
	f_next_token(parser->lexer);

	/* parse all statements until we get a closed brace */
	for (;;) {
		if (parser->lexer->curr_token.type == TOK_BRACE_CLOSED) {
			/* skip closing '}' */
			f_next_token(parser->lexer);

			/* leave current scope */
			sym_pop_scope(parser->sym_table);

			return left;
		}

		/* parse_statement mat call parse_comound_statement,
		 * so it is called recursively in nested comound statements */
		tree = parse_statement(parser);

		if (tree) {
			/* if its the first statement */
			if (!left) {
				left = tree;
			} else {
				/* glue together with prev statements */
				left = make_ast_node(parser, AST_NOP, left, NULL, tree);
			}
		}
	}
}

static ast_node_t*
parse_function_body(parser_t *parser)
{

}



void
f_create_parser(parser_t *parser, lexer_t *lexer, sym_table_t *table)
{
	parser->lexer		= lexer;
	parser->sym_table	= table;
	parser->pool		= mem_pool_create(1024);
}


void
f_destroy_parser(parser_t *parser)
{
	mem_pool_destroy(&parser->pool);
}


ast_node_t*
f_generate_ast(parser_t *parser)
{
	lex_token_t token = parser->lexer->curr_token;
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

				function = parse_global_declaration(parser);

				if (function) {
					return function;
				}

				break;

			case TOK_EOF:
				return NULL;

			default:
				// printf("parsing token: %s\n", lex_tok_debug_str(token.type));
				printf("got here\n");
				syntax_error(parser->lexer->curr_token.err_loc, "failure parsing");
		}
	}

}
