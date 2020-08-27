#include "lex.h"

#include <stdio.h>

int main()
{
	struct lex_file_buffer file_buffer;
	lex_load_file_buffer(&file_buffer, "../test/test.c");

	printf("File Size: %lu bytes \n", file_buffer.end - file_buffer.start);

	struct lex_token token = lex_next_token(&file_buffer);

	while (token.type != TOK_EOF)
	{
		token = lex_next_token(&file_buffer);
	}

	lex_destroy_file_buffer(&file_buffer);

	return 0;
}
