#include "f_lexer.h"
#include "f_parser.h"
#include "f_ast.h"

#include <stdio.h>
#include <assert.h>

#if 0
// run the lexer on a list of tokens
static void
run_test1()
{
	struct lex_instance lex_in = lex_create_instance("../test/test.c");

	struct lex_token token;

	token = lex_next_token(&lex_in);

	printf("Running test 1 : lex token type...\n");
	for (uint32_t i = 0; i < _TOK_COUNT; ++i) {
		token = lex_next_token(&lex_in);

		if ((uint32_t)token.type == i) {
			printf("Success : %s\n", lex_tok_debug_str(i));
		} else {
			printf("Failure : %s : %s\n", lex_tok_debug_str(i), lex_tok_debug_str(token.type));
		}
	}

	lex_destroy_instance(&lex_in);
}

// run the lexer on a real source file
static void
run_test2()
{
	static enum lex_token_type expected_tokens[] = 
	{
		TOK_KEY_TYPEDEF, TOK_KEY_STRUCT, TOK_IDENTIFIER,
		TOK_BRACE_OPEN, TOK_KEY_VOLATILE, TOK_KEY_LONG,
		TOK_IDENTIFIER, TOK_SEMIKOLON, TOK_KEY_CONST,
		TOK_KEY_SHORT, TOK_IDENTIFIER, TOK_SEMIKOLON,
		TOK_KEY_CHAR, TOK_IDENTIFIER, TOK_SEMIKOLON,
		TOK_KEY_UNION, TOK_BRACE_OPEN, TOK_KEY_INT,
		TOK_IDENTIFIER, TOK_SEMIKOLON, TOK_KEY_FLOAT,
		TOK_IDENTIFIER, TOK_SEMIKOLON, TOK_KEY_DOUBLE,
		TOK_IDENTIFIER, TOK_SEMIKOLON, TOK_BRACE_CLOSED,
		TOK_SEMIKOLON, TOK_KEY_ENUM, TOK_BRACE_OPEN,
		TOK_IDENTIFIER, TOK_COMMA, TOK_IDENTIFIER,
		TOK_BRACE_CLOSED, TOK_SEMIKOLON, TOK_BRACE_CLOSED,
		TOK_IDENTIFIER, TOK_SEMIKOLON,
		TOK_KEY_INT, TOK_IDENTIFIER, TOK_PAREN_OPEN,
		TOK_KEY_INT, TOK_IDENTIFIER, TOK_COMMA, TOK_KEY_CHAR,
		TOK_STAR, TOK_STAR, TOK_IDENTIFIER, TOK_PAREN_CLOSED,
		TOK_BRACE_OPEN, TOK_IDENTIFIER, TOK_IDENTIFIER,
		TOK_SEMIKOLON, TOK_IDENTIFIER, TOK_DOT,
		TOK_IDENTIFIER, TOK_ASSIGN, TOK_LITERAL,
		TOK_DIV, TOK_LITERAL, TOK_SEMIKOLON,
		TOK_KEY_RETURN, TOK_LITERAL, TOK_SEMIKOLON,
		TOK_BRACE_CLOSED
	};

	struct lex_instance lex_in = lex_create_instance("../test/test2.c");

	struct lex_token token;
	int i = 0;

	for (
		i = 0;
		i < (uint32_t)sizeof(expected_tokens) / (uint32_t)sizeof(enum lex_token_type);
		++i
		) {
		token = lex_next_token(&lex_in);

		if (expected_tokens[i] != token.type) {
			printf("Failure : %s, i = %i : should have gotten : %s\n",
					lex_tok_debug_str(token.type),
					i, lex_tok_debug_str(expected_tokens[i]));
		} else {
			printf("Success : %s, i = %i\n", lex_tok_debug_str(token.type), i);
		}
	}

	// sym_cleanup_table(&table);
	lex_destroy_instance(&lex_in);
}

static void
run_test3()
{
	struct lex_instance lex_in = lex_create_instance("../test/test3.c");

	struct lex_token token;
	static char expected_values[] = {
		'a', 'b', '\n', '\0', '\\', '\'', '\a', '\123', '\x12'
	};

	for (uint32_t i = 0; i < 9; ++i) {
		token = lex_next_token(&lex_in);
		if (expected_values[i] != (char)token.literal.value.val_int) {
			printf("Failure : expected = %i, result = %i\n",
					expected_values[i], (int32_t)token.literal.value.val_int);
		} else {
			printf("Success : %i\n", expected_values[i]);
		}
	}

	lex_destroy_instance(&lex_in);
}

#endif

int main()
{
	ast_node_t *tree;

	sym_table_t table;
	lexer_t lexer;
	parser_t parser;

	sym_create_table(&table, 32);
	f_create_lexer(&lexer, "../test/test5.c");
	f_create_parser(&parser, &lexer, &table);

	printf("%i\n", f_next_token(&lexer).type);

	printf("line %u\n", lexer.col);

	tree = f_generate_ast(&parser);

	while (tree) {

		printf("== FUNC ==\n");
		f_print_ast(tree, 0);
		printf("\n");
	
		tree = f_generate_ast(&parser);
	}

	sym_destroy_table(&table);
	f_destroy_lexer(&lexer);
	f_destroy_parser(&parser);

	return 0;
}
