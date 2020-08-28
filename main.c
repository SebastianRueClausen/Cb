#include "lex.h"

#include <stdio.h>

static void
run_test1()
{
	struct lex_file_buffer file_buffer;
	lex_load_file_buffer(&file_buffer, "../test/test.c");

	struct lex_token token;

	lex_next_token(&file_buffer);

	printf("Running test 1 : lex token type...\n");
	for (int i = 0; i < _TOK_COUNT; ++i)
	{
		token = lex_next_token(&file_buffer);

		if ((int)token.type == i) {
			printf("Success : %s\n", tok_to_str[i]);
		}
		else {
			printf("Failure : %s : %s\n",
					tok_to_str[i], tok_to_str[(int)token.type]);
		}
	}

	lex_destroy_file_buffer(&file_buffer);
}

int main()
{
	run_test1();

	return 0;
}
