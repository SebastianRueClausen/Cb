#include "f_parser.h"
#include "f_lexer.h"
#include "f_expr.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

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
	lex_token_type_t				type = parser->lexer->curr_token.type;
	uint32_t						n = 0;


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
	type_info_t						type = NULL_TYPE_INFO;


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
	sym_param_t						param;
	vec_sym_param_t					vec = vec_sym_param_t_create(4);


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
	sym_global_t				global;
	sym_hash_t					hash;


	global.type = parse_type(parser);

	type_check_validity(&global.type, &parser->lexer->curr_token.err_loc);

	hash = parser->lexer->curr_token.hash;

	parse_token(parser, TOK_IDENTIFIER);

	switch (parser->lexer->curr_token.type) {

		/* def of global var */
		case TOK_ASSIGN:

			/* @todo: add some kind of compile time expression parser */
			f_parse_expression(parser, 0, TOK_SEMIKOLON);

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
	sym_id_t					id;
	sym_hash_t					hash;
	sym_local_t					local;
	ast_node_t					*right;
	ast_node_t					*left;

	
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
			right = f_make_ast_node(parser, AST_IDENTIFIER, NULL, NULL, NULL);
			right->sym_id = id;

			/* parse the expression */
			left = f_parse_expression(parser, 0, TOK_SEMIKOLON);

			/* skip semikolon */
			f_next_token(parser->lexer);

			/* glue together expression */
			left = f_make_ast_node(parser, AST_ASSIGN, left, NULL, right);

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
	ast_node_t					*condition_ast;
	ast_node_t					*true_ast;
	ast_node_t					*false_ast;


	/* skip 'if' token */
	f_next_token(parser->lexer);

	/* skip opening '(' */
	parse_token(parser, TOK_PAREN_OPEN);

	/* parse conditional expression */
	condition_ast = f_parse_expression(parser, 0, TOK_PAREN_CLOSED);

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

	return f_make_ast_node(parser, AST_IF, condition_ast, true_ast, false_ast);
}


static ast_node_t*
parse_while_loop(parser_t *parser)
{
	ast_node_t					*condition_ast;
	ast_node_t					*body_ast;


	/* skip 'while' token */
	f_next_token(parser->lexer);

	/* skip the '(' */
	parse_token(parser, TOK_PAREN_OPEN);

	/* parse the conditional */
	condition_ast = f_parse_expression(parser, 0, TOK_PAREN_CLOSED);

	/* skip closing ')' */
	f_next_token(parser->lexer);

	/* parse the body */
	body_ast = parse_statement(parser);

	/* glue body and conditional together with a while ast */
	return f_make_ast_node(parser, AST_WHILE, condition_ast, NULL, body_ast);

}


static ast_node_t*
parse_for_loop(parser_t *parser)
{
	ast_node_t				   *tree;
	ast_node_t				   *body;
	ast_node_t				   *cond;
	ast_node_t				   *pre_op;
	ast_node_t				   *post_op;


	/* skip opening 'for' token */
	f_next_token(parser->lexer);

	parse_token(parser, TOK_PAREN_OPEN);

	/* parse preop expression */
	pre_op = f_parse_expression(parser, 0, TOK_SEMIKOLON);
	f_next_token(parser->lexer);

	/* parse conditional statement */
	cond = f_parse_expression(parser, 0, TOK_SEMIKOLON);
	f_next_token(parser->lexer);

	/* parse post op */
	post_op = f_parse_expression(parser, 0, TOK_PAREN_CLOSED);
	f_next_token(parser->lexer);

	/* parse statement */
	body = parse_statement(parser);

	/* glue the body and post op so we does the postop after the body */
	tree = f_make_ast_node(parser, AST_NOP, body, NULL, post_op);

	/* we make a while loop with the body and cond */
	tree = f_make_ast_node(parser, AST_WHILE, cond, NULL, tree);

	/* glue the preop and while loop so we does the preop before the loop */
	tree = f_make_ast_node(parser, AST_NOP, pre_op, NULL, tree);

	return tree;
}


/* creates an ast tree from a single statement, which can include compound statements */
/* expect the current token to be the first of the statement */
ast_node_t*
parse_statement(parser_t *parser)
{
	ast_node_t					*tree;
	lex_token_t					token = parser->lexer->curr_token;
	

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
			tree = f_parse_expression(parser, 0, TOK_SEMIKOLON);

			/* skip semikolon */
			f_next_token(parser->lexer);

			return tree;
	}
}


/* loops through all statements in a compound statement and creates an ast tree */
static ast_node_t*
parse_compound_statement(parser_t *parser)
{
	ast_node_t					*left = NULL;
	ast_node_t					*tree = NULL;


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
				left = f_make_ast_node(parser, AST_NOP, left, NULL, tree);
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
	parser->lexer				= lexer;
	parser->sym_table			= table;
	parser->pool				= mem_pool_create(1024);
}


void
f_destroy_parser(parser_t *parser)
{
	mem_pool_destroy(&parser->pool);
}


ast_node_t*
f_generate_ast(parser_t *parser)
{
	ast_node_t*					func;
	lex_token_t					token = parser->lexer->curr_token;


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

				func = parse_global_declaration(parser);

				if (func) {
					return func;
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
