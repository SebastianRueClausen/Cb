#include "lex.h"
#include "ast.h"

#include <stdio.h>

// run the lexer on a list of tokens
static void
run_test1()
{
	struct lex_file_buffer file_buffer;
	lex_load_file_buffer(&file_buffer, "../test/test.c");

	struct lex_token token;

	lex_next_token(&file_buffer, &token);

	printf("Running test 1 : lex token type...\n");
	for (int i = 0; i < _TOK_COUNT; ++i)
	{
		lex_next_token(&file_buffer, &token);

		if ((int)token.type == i) {
			printf("Success : %s\n", lex_tok_to_str[i]);
		}
		else {
			printf("Failure : %s : %s\n",
					lex_tok_to_str[i], lex_tok_to_str[(int)token.type]);
		}
	}

	lex_destroy_file_buffer(&file_buffer);
}

// run the lexer on a real source file
static void
run_test2()
{
	static enum lex_token_type expected_tokens[] = 
	{
		TOK_KEY_TYPEDEF, TOK_KEY_STRUCT, TOK_IDENTIFIER,
		TOK_CTRL_BRACE_OPEN, TOK_KEY_VOLATILE, TOK_KEY_LONG,
		TOK_IDENTIFIER, TOK_CTRL_SEMIKOLON, TOK_KEY_CONST,
		TOK_KEY_SHORT, TOK_IDENTIFIER, TOK_CTRL_SEMIKOLON,
		TOK_KEY_CHAR, TOK_IDENTIFIER, TOK_CTRL_SEMIKOLON,
		TOK_KEY_UNION, TOK_CTRL_BRACE_OPEN, TOK_KEY_INT,
		TOK_IDENTIFIER, TOK_CTRL_SEMIKOLON, TOK_KEY_FLOAT,
		TOK_IDENTIFIER, TOK_CTRL_SEMIKOLON, TOK_KEY_DOUBLE,
		TOK_IDENTIFIER, TOK_CTRL_SEMIKOLON, TOK_CTRL_BRACE_CLOSED,
		TOK_CTRL_SEMIKOLON, TOK_KEY_ENUM, TOK_CTRL_BRACE_OPEN,
		TOK_IDENTIFIER, TOK_CTRL_COMMA, TOK_IDENTIFIER,
		TOK_CTRL_BRACE_CLOSED, TOK_CTRL_SEMIKOLON, TOK_CTRL_BRACE_CLOSED,
		TOK_IDENTIFIER, TOK_CTRL_SEMIKOLON,
		TOK_KEY_INT, TOK_IDENTIFIER, TOK_CTRL_PAREN_OPEN,
		TOK_KEY_INT, TOK_IDENTIFIER, TOK_CTRL_COMMA, TOK_KEY_CHAR,
		TOK_OP_STAR, TOK_OP_STAR, TOK_IDENTIFIER, TOK_CTRL_PAREN_CLOSED,
		TOK_CTRL_BRACE_OPEN, TOK_IDENTIFIER, TOK_IDENTIFIER,
		TOK_CTRL_SEMIKOLON, TOK_IDENTIFIER, TOK_CTRL_DOT,
		TOK_IDENTIFIER, TOK_OP_ASSIGN, TOK_CONSTANT_FLOAT,
		TOK_OP_DIV, TOK_CONSTANT_INT, TOK_CTRL_SEMIKOLON,
		TOK_KEY_RETURN, TOK_CONSTANT_INT, TOK_CTRL_SEMIKOLON,
		TOK_CTRL_BRACE_CLOSED
	};

	struct lex_file_buffer file_buffer;
	lex_load_file_buffer(&file_buffer, "../test/test2.c");

	struct lex_token token;

	for (
			int i = 0;
			i < (int)sizeof(expected_tokens) / (int)sizeof(enum lex_token_type);
			++i
		)
	{
		lex_next_token(&file_buffer, &token);

		if (expected_tokens[i] != token.type) {
			printf("Failure : %s, i = %i : should have gotten : %s\n",
					lex_tok_to_str[(int)token.type],
					i, lex_tok_to_str[expected_tokens[i]]);
		}
		else {
			printf("Success : %s, i = %i\n",
					lex_tok_to_str[(int)token.type], i);
		}
	}

	lex_destroy_file_buffer(&file_buffer);
}

static void
run_test3()
{
	struct lex_file_buffer file_buffer;
	lex_load_file_buffer(&file_buffer, "../test/test3.c");

	struct lex_token token;

	static char expected_values[] = {
		'a', 'b', '\n', '\0', '\\', '\'', '\a', '\123', '\x12'
	};

	for (int i = 0; i < 9; ++i) {
		lex_next_token(&file_buffer, &token);
		if (expected_values[i] != token.value_char) {
			printf("Failure : expected = %i, result = %i\n", expected_values[i], token.value_char);
		}
		else {
			printf("Success : %i\n", expected_values[i]);
		}
	}

	lex_destroy_file_buffer(&file_buffer);
}

extern struct ast_node*
parse_expression(struct lex_file_buffer* fb, int prev_prec);

static void
run_test4()
{
	struct lex_file_buffer file_buffer;
	struct lex_token token;

	lex_load_file_buffer(&file_buffer, "../test/test4.c");

	parse_expression(&file_buffer, 0);
	
}

int main()
{
	// run_test1();
	// run_test2();
	// run_test3();
	run_test4();
	
	return 0;
}
