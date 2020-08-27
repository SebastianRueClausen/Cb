#include "lex.h"
#include "gen.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

void
lex_load_file_buffer(
	struct lex_file_buffer* file_buffer, const char* filename
)
{
	FILE* file = fopen(filename, "r");
	assert(file);

	fseek(file, 0, SEEK_END);
	const size_t file_size = ftell(file);
	rewind(file);

	file_buffer->start = (char*) c_malloc(file_size);
	assert(file_buffer->start);

	file_buffer->curr = file_buffer->start;
	file_buffer->end = file_buffer->start + file_size;
	file_buffer->line = 0;

	size_t result = fread(file_buffer->start, 1, file_size, file);
	assert(result);

	fclose(file);
}

void
lex_destroy_file_buffer(struct lex_file_buffer* file_buffer)
{
	assert(file_buffer);
	assert(file_buffer->start);

	c_free(file_buffer->start);

	file_buffer->start	= NULL;
	file_buffer->curr	= NULL;
	file_buffer->end	= NULL;
}

static char
next(struct lex_file_buffer* file_buffer)
{
	char c;

	if (file_buffer->curr != file_buffer->end) {
		c = *(++file_buffer->curr);
	}
	else {
		return '\0';
	}

	if (c == '\n') {
		++file_buffer->line;
	}

	return c;
}

static char
peek(struct lex_file_buffer* const file_buffer, uint32_t n)
{
	char c;

	if (file_buffer->curr + n <= file_buffer->end) {
		c = file_buffer->curr[n];
	}
	else {
		c = '\0';
	}
	
	return c;
}

static bool
cmp(struct lex_file_buffer* const file_buffer, const char* str)
{
	uint32_t i = 0;

	while (str[i]) {
		if (str[i] != peek(file_buffer, i))
			return false;

		++i;
	}

	return true;
}

static char
skip_single_line_comment(struct lex_file_buffer* file_buffer)
{
	char c = *file_buffer->curr;

	while (c != '\n' && c != '\0') {
		c = next(file_buffer);
	}

	c = next(file_buffer);

	return c;
}

static char
skip_multi_line_comment(struct lex_file_buffer* file_buffer)
{
	char c = *file_buffer->curr;

	// skip opening /*
	c = next(file_buffer);
	c = next(file_buffer);

	while (!cmp(file_buffer, "*/")) {
		c = next(file_buffer);	
		
		if (!c) {
			fatal_error("Syntax Error: Unexpected end of file in comment\n");
		}
	}

	// skip closing */
	c = next(file_buffer);
	c = next(file_buffer);
	
	return c;
}

static char
skip_whitespace_and_comments(struct lex_file_buffer* file_buffer)
{
	char c;

	c = *file_buffer->curr;;

	for (;;) {
		switch (c) {
			case ' ': case '\n': case '\t':
			case '\r': case '\v': case '\f': {
				c = next(file_buffer);
			}
			break;

			case '/': {
				if (cmp(file_buffer, "/*")) {
					c = skip_multi_line_comment(file_buffer);
				}
				else if (cmp(file_buffer, "//")) {
					c = skip_single_line_comment(file_buffer);
				}
			}
			break;

			default: {
				return c;
			}
		}
	}
}

static bool
is_numeric(char c)
{
	if (c >= '0' && c <= '9') {
		return true;
	}
	else {
		return false;
	}
}

static bool
is_letter(char c)
{
	if (c >= 'A' && c <= 'Z')
		return true;

	if (c >= 'a' && c <= 'z')
		return true;

	if (c == '_')
		return true;

	return false;
}

static uint32_t
word_len(const struct lex_file_buffer* file_buffer)
{
	char* str = file_buffer->curr;
	uint32_t i = 0;
	
	while (str[i]) {
		if (!is_letter(str[i]) && !is_numeric(str[i]))
			return i;
		++i;
	}

	assert(false);
}

static uint32_t
number_len(const struct lex_file_buffer* file_buffer)
{
	char* str = file_buffer->curr;
	uint32_t i = 0;

	while (str[i]) {
		if (!is_numeric(str[i]) && str[i] != '.')
			return i;
		++i;
	}

	assert(false);
}

static enum lex_token_type
parse_numeric_constant(const char* str, uint32_t len, int64_t* val)
{
	uint32_t i = 0;
	size_t tmp = 0;
	double d_tmp = 0;
	double k = 0.1;

	while (i != len && str[i] != '.') {
		tmp = tmp * 10 + str[i] - '0';
		++i;
	}
	
	if (str[i] != '.') {
		*val = tmp;
		return TOK_CONSTANT_INT;
	}

	++i;
	d_tmp = (double)tmp;

	while (i != len) {
		d_tmp += (str[i] - '0') * k;
		k *= 0.1;
		++i;
	}

	*((double*)val) = d_tmp;
	return TOK_CONSTANT_FLOAT;
}

static enum lex_token_type
lookup_keyword(const char* str, uint32_t len)
{
	switch (*str) {
		case 'i':
			if (!strncmp(str, "if", len)) {
				printf("len %u\n", len);
				return TOK_KEY_IF;
			}
			if (!strncmp(str, "int", len))
				return TOK_KEY_INT;
			break;
		case 'e':
			if (!strncmp(str, "else", len))
				return TOK_KEY_ELSE;
			if (!strncmp(str, "enum", len))
				return TOK_KEY_ENUM;
			if (!strncmp(str, "extern", len))
				return TOK_KEY_EXTERN;
			break;
		case 'w':
			if (!strncmp(str, "while", len))
				return TOK_KEY_WHILE;
			break;
		case 'f':
			if (!strncmp(str, "for", len))
				return TOK_KEY_FOR;
			if (!strncmp(str, "float", len))
				return TOK_KEY_FLOAT;
			break;
		case 's':
			if (!strncmp(str, "switch", len))
				return TOK_KEY_SWITCH;
			if (!strncmp(str, "struct", len))
				return TOK_KEY_STRUCT;
			if (!strncmp(str, "static", len))
				return TOK_KEY_STATIC;
			if (!strncmp(str, "sizeof", len))
				return TOK_KEY_SIZEOF;
			break;
		case 'c':
			if (!strncmp(str, "char", len))
				return TOK_KEY_CHAR;
			if (!strncmp(str, "const", len))
				return TOK_KEY_CONST;
			if (!strncmp(str, "case", len))
				return TOK_KEY_CASE;
			if (!strncmp(str, "continue", len))
				return TOK_KEY_CONTINUE;
			break;
		case 'b':
			if (!strncmp(str, "break", len))
				return TOK_KEY_BREAK;
			break;
		case 'r':
			if (!strncmp(str, "return", len))
				return TOK_KEY_RETURN;
			break;
		case 'd':
			if (!strncmp(str, "double", len))
				return TOK_KEY_DOUBLE;
			if (!strncmp(str, "default", len))
				return TOK_KEY_DEFAULT;
			if (!strncmp(str, "do", len))
				return TOK_KEY_DO;
			break;
		case 'l':
			if (!strncmp(str, "long", len))
				return TOK_KEY_LONG;
			break;
		case 'u':
			if (!strncmp(str, "union", len))
				return TOK_KEY_UNION;
			break;
		case 't':
			if (!strncmp(str, "typedef", len))
				return TOK_KEY_TYPEDEF;
			break;
		case 'v':
			if (!strncmp(str, "void", len))
				return TOK_KEY_VOID;
			break;
		case 'g':
			if (!strncmp(str, "goto", len))
				return TOK_KEY_GOTO;
			break;
	}

	return TOK_IDENTIFIER;
}


static enum lex_token_type
lookup_symbol(const char* str)
{
	switch (*str) {
		case '+':
			if (str[1] == '=')
				return TOK_OP_PLUS_ASSIGN;
			else if (str[1] == '+')
				return TOK_OP_INCREASE;
			return TOK_OP_PLUS;
		case '-':
			if (str[1] == '=')
				return TOK_OP_MINUS_ASSIGN;
			else if (str[1] == '-')
				return TOK_OP_DECREASE;
			else if (str[1] == '>')
				return TOK_CTRL_ARROW;
			return TOK_OP_MINUS;
		case '/':
			if (str[1] == '=')
				return TOK_OP_DIV_ASSIGN;
			return TOK_OP_DIV;
		case '*':
			if (str[1] == '=')
				return TOK_OP_STAR_ASSIGN;
			return TOK_OP_STAR;
		case '%':
			if (str[1] == '=')
				return TOK_OP_MOD_ASSIGN;
			return TOK_OP_MOD;
		case '=':
			if (str[1] == '=')
				return TOK_LOG_EQUAL;
			return TOK_OP_ASSIGN;
		case '&':
			if (str[1] == '=')
				return TOK_BIT_AMPERSAND_ASSIGN;
			else if (str[1] == '&')
				return TOK_LOG_AND;
			return TOK_BIT_AMPERSAND;
		case '|':
			if (str[1] == '=')
				return TOK_BIT_OR_ASSIGN;
			if (str[1] == '|')
				return TOK_LOG_OR;
			return TOK_BIT_OR;
		case '~':
			if (str[1] == '=')
				return TOK_BIT_NOT_ASSIGN;
			return TOK_BIT_NOT;
		case '^':
			if (str[1] == '=')
				return TOK_BIT_EOR_ASSIGN;
			return TOK_BIT_EOR;
		case '>':
			if (str[1] == '=')
				return TOK_LOG_GREATER_OR_EQUAL;
			else if (str[1] == '>') {
				if (str[2] == '=')
					return TOK_BIT_RIGHT_SHIFT_ASSIGN;
				else
					return TOK_BIT_RIGHT_SHIFT;
			}
			return TOK_LOG_GREATER;
		case '<':
			if (str[1] == '=')
				return TOK_LOG_LESSER_OR_EQUAL;
			else if (str[1] == '<') {
				if (str[2] == '=')
					return TOK_BIT_LEFT_SHIFT_ASSIGN;
				else
					return TOK_BIT_LEFT_SHIFT;
			}
			return TOK_LOG_LESSER;
		case '(':
			return TOK_CTRL_PAREN_OPEN;
		case ')':
			return TOK_CTRL_PAREN_CLOSED;
		case '[':
			return TOK_CTRL_BRACKET_OPEN;
		case ']':
			return TOK_CTRL_BRACKET_CLOSED;
		case '{':
			return TOK_CTRL_BRACE_OPEN;
		case '}':
			return TOK_CTRL_BRACE_CLOSED;
		case '!':
			if (str[1] == '=')
				return TOK_LOG_NOT_EQUAL;
			return TOK_LOG_EQUAL;
		case ',':
			return TOK_CTRL_COMMA;
		case '.':
			if (str[1] == '.') {
				if (str[2] == '.')
					return TOK_CTRL_ELLIPSIS;
			}
			return TOK_CTRL_DOT;
		case ';':
			return TOK_CTRL_SEMIKOLON;
		case ':':
			return TOK_CTRL_KOLON;
		case '?':
			return TOK_CTRL_QUERY;
	}

	return TOK_UNKNOWN;
}

struct lex_token
lex_next_token(struct lex_file_buffer* file_buffer)
{
	char c;
	uint32_t token_len;
	struct lex_token token;

	c = skip_whitespace_and_comments(file_buffer);

	if (is_letter(c)) {
		token_len = word_len(file_buffer);
		token.type = lookup_keyword(file_buffer->curr, token_len);

		file_buffer->curr += token_len;		
	}
	else if (is_numeric(c)) {
		token_len = number_len(file_buffer);	
		token.type = parse_numeric_constant(file_buffer->curr, 
				token_len, &token.value_int);
		file_buffer->curr += token_len;
	}
	else {
		token.type = TOK_EOF;
	}

	return token;
}
