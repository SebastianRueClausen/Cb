#include "lex.h"
#include "gen.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

void
lex_load_file_buffer(struct lex_file_buffer* file_buffer,
							const char* filename)
{
	size_t filename_size;
	size_t file_size;
	FILE* file = fopen(filename, "r");
	
	if (!file) {
		fatal_error("%s: No such file", filename);
	}

	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	rewind(file);

	file_buffer->start = (char*)c_malloc(file_size);
	file_buffer->curr = file_buffer->start;
	file_buffer->end = file_buffer->start + file_size;

	filename_size = strlen(filename);
	file_buffer->filename = (char*)c_malloc(filename_size);
	strncpy(file_buffer->filename, filename, filename_size);

	file_buffer->line = 0;
	file_buffer->col = 0;

	if (!fread(file_buffer->start, 1, file_size, file)) {
		fatal_error("Failure reading %s", filename);
	}

	fclose(file);
}

void
lex_destroy_file_buffer(struct lex_file_buffer* file_buffer)
{
	c_free(file_buffer->start);
	c_free(file_buffer->filename);

	file_buffer->col	= 0;
	file_buffer->line	= 0;
	file_buffer->start	= NULL;
	file_buffer->curr	= NULL;
	file_buffer->end	= NULL;
}

static struct err_location
error_location(const struct lex_file_buffer* file_buffer)
{
	struct err_location loc = {
		.line		= file_buffer->line,
		.col		= file_buffer->col,
		.filename	= file_buffer->filename
	};

	return loc;
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
		file_buffer->col = 0;
	}
	else {
		++file_buffer->col;
	}

	return c;
}

static void
skip(struct lex_file_buffer* file_buffer, uint32_t n)
{
	for (uint32_t i = 0; i < n; ++i) {
		next(file_buffer);
	}
}

static bool
str_cmp(const char* str1, const char* str2, uint32_t len1, uint32_t len2)
{
	if (len1 != len2)
		return false;

	for (uint32_t i = 0; i < len1; ++i) {
		if (str1[i] != str2[i])
			return false;
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
	// skip opening /*
	skip(file_buffer, 2);

	while (file_buffer->curr != file_buffer->end) {
		if (file_buffer->curr[0] == '*') {
			if (file_buffer->curr[1] == '/') {
				skip(file_buffer, 2);
				return *file_buffer->curr;
			}
		}
		next(file_buffer);
	}

	syntax_error(error_location(file_buffer),
			"Unexpected end of file in comment");
	return '\0';
}

static char
skip_whitespace_and_comments(struct lex_file_buffer* file_buffer)
{
	for (;;) {
		switch (*file_buffer->curr) {
			case ' ': case '\n': case '\t':
			case '\r': case '\v': case '\f': {
				next(file_buffer);
			}
			break;

			case '/': {
				if (file_buffer->curr[1] == '*') {
					skip_multi_line_comment(file_buffer);
				}
				else if (file_buffer->curr[1] == '/') {
					skip_single_line_comment(file_buffer);
				}
				else {
					return *file_buffer->curr;
				}
			}
			break;

			default: {
				return *file_buffer->curr;
			}
		}
	}
}

static uint32_t
word_len(const struct lex_file_buffer* file_buffer)
{
	uint32_t i = 0;
	const char* str = file_buffer->curr;
	
	while (str[i]) {
		if (isalpha(str[i]) || str[i] == '_' || isdigit(str[i]))
			++i;
		else
			return i;
	}

	syntax_error(error_location(file_buffer), "Unexpected end of file");
	return 0;
}

static uint32_t
number_len(const struct lex_file_buffer* file_buffer)
{
	uint32_t i = 0;
	const char* str = file_buffer->curr;

	while (str[i]) {
		if (isdigit(str[i]) || str[i] == '.')
			++i;
		else
			return i;
	}

	syntax_error(error_location(file_buffer), "Unexpected end of file");
	return 0;
}

static uint32_t
string_len(const struct lex_file_buffer* file_buffer)
{
	uint32_t i = 0;
	const char* str = file_buffer->curr;

	while (str[i]) {
		if (str[i] != '"')
			++i;
		else
			return i;
	}

	syntax_error(error_location(file_buffer),
			"Unexpected end of file inside string");
	return 0;
}

static uint32_t
char_len(const struct lex_file_buffer* file_buffer)
{
	uint32_t i = 0, slashes = 0;
	const char* str = file_buffer->curr;

	while (str[i]) {
		if (str[i] == '\'') {
			if (slashes == 1) {
				slashes = 0;
				++i;
				continue;
			}

			return i;
		}
		else if (str[i] == '\\') {
			++slashes;
			++i;
		}
		else {
			slashes = 0;
			++i;
		}
	}

	syntax_error(error_location(file_buffer), 
			"Unexpected end of file inside char literal");

	return 0;
}

static enum lex_token_type
parse_decimal_constant(const char* str, uint32_t len, int64_t* val)
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
parse_octal_constant(const char* str, uint32_t len, int64_t* val)
{
	uint32_t i = 0;
	size_t tmp = 0;
	double d_tmp = 0;
	double k = 0.125;

	while (i != len && str[i] != '.') {
		tmp = tmp * 8 + str[i] - '0';
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
		k *= 0.125;
		++i;
	}

	*((double*)val) = d_tmp;
	return TOK_CONSTANT_FLOAT;
}

static enum lex_token_type
parse_hex_constant(const char* str, uint32_t len, int64_t* val)
{
	uint32_t i = 0;
	size_t tmp = 0;

	while (i != len) {

		if (str[i] >= 'a' && str[i] <= 'f')
			tmp = tmp * 16 + str[i] - 0x57;
		else if (str[i] >= 'A' && str[i] <= 'F')
			tmp = tmp * 16 + str[i] - 0x37;
		else
			tmp = tmp * 16 + str[i] - '0';

		++i;
	}
	
	*val = tmp;

	// TODO add floating point hex literals
	
	return TOK_CONSTANT_INT;
}

static void
parse_string_literal(const char* str, uint32_t len, char* buffer)
{
	
}

static int
parse_multichar_constant(const struct lex_file_buffer* file_buffer, int len)
{
	int i, val = 0;
	const char* str = file_buffer->curr;

	for (i = 0; i < len; ++i) {
		val = str[i];	
	}

	syntax_warning(error_location(file_buffer),
		"multi-character constant");

	if (len >= 6)
		syntax_warning(error_location(file_buffer),
			"multi-character constant is too long");

	return i;
}

static int
parse_char_literal(const struct lex_file_buffer* file_buffer, int len)
{
	int64_t c = 0;
	const char* str = file_buffer->curr;

	switch (len) {
		case 1:
			return str[0];
		case 2:
			if (str[0] == '\\') {
				switch (str[1]) {
					case 'a':	return '\a';
					case 'b':	return '\b';
					case 'f':	return '\f';
					case 'r':	return '\r';
					case 'n':	return '\n';
					case 't':	return '\t';
					case 'v':	return '\v';
					case '\\':	return '\\';
					case '\'':	return '\'';

					case '0': case '1': case '2':
					case '3': case '4': case '5':
					case '6': case '7':
						parse_octal_constant(str + 1, len - 1, &c);
						return c;
					case 'x':
						parse_hex_constant(str + 2, len - 2, &c);
						return c;
					default:
						syntax_error(error_location(file_buffer),
								"Invalid escape sequence");
				}
			}
			else {
				return parse_multichar_constant(file_buffer, len);		
			}
			break;

		case 3: case 4:
			if (str[0] == '\\') {
				switch (str[1]) {
					case '0': case '1': case '2':
					case '3': case '4': case '5':
					case '6': case '7':
						parse_octal_constant(str + 1, len - 1, &c);
						return c;
					case 'x':
						parse_hex_constant(str + 2, len - 2, &c);
						return c;
					case 'u':
						syntax_error(error_location(file_buffer),
								"Unicode is yet to be implemented");
						return 0;
					default:
						syntax_error(error_location(file_buffer),
								"Invalid escape sequence");
						return 0;
				}
			}
			else {
				return parse_multichar_constant(file_buffer, len);
			}

		case 8:
			if (str[0] == '\\') {
				if (str[1] == 'U') {
					syntax_error(error_location(file_buffer),
							"Unicode is yet to be implemented");
				}
			}
			else {
				return parse_multichar_constant(file_buffer, len);
			}

		default:
			return parse_multichar_constant(file_buffer, len);
	}
	
	printf("We shouldent be here");
	assert(false);
}

static enum lex_token_type
lookup_keyword(const char* str, uint32_t len)
{
	switch (*str) {
		case 'i':
			if (str_cmp(str, "if", len, 2))
				return TOK_KEY_IF;
			if (str_cmp(str, "int", len, 3))
				return TOK_KEY_INT;
			break;
		case 'e':
			if (str_cmp(str, "else", len, 4))
				return TOK_KEY_ELSE;
			if (str_cmp(str, "enum", len, 4))
				return TOK_KEY_ENUM;
			if (str_cmp(str, "extern", len, 6))
				return TOK_KEY_EXTERN;
			break;
		case 'w':
			if (str_cmp(str, "while", len, 5))
				return TOK_KEY_WHILE;
			break;
		case 'f':
			if (str_cmp(str, "for", len, 3))
				return TOK_KEY_FOR;
			if (str_cmp(str, "float", len, 5))
				return TOK_KEY_FLOAT;
			break;
		case 's':
			if (str_cmp(str, "switch", len, 6))
				return TOK_KEY_SWITCH;
			if (str_cmp(str, "struct", len, 6))
				return TOK_KEY_STRUCT;
			if (str_cmp(str, "static", len, 6))
				return TOK_KEY_STATIC;
			if (str_cmp(str, "sizeof", len, 6))
				return TOK_KEY_SIZEOF;
			if (str_cmp(str, "short", len, 5))
				return TOK_KEY_SHORT;
			if (str_cmp(str, "signed", len, 6))
				return TOK_KEY_SIGNED;;
			break;
		case 'c':
			if (str_cmp(str, "char", len, 4))
				return TOK_KEY_CHAR;
			if (str_cmp(str, "const", len, 5))
				return TOK_KEY_CONST;
			if (str_cmp(str, "case", len, 4))
				return TOK_KEY_CASE;
			if (str_cmp(str, "continue", len, 8))
				return TOK_KEY_CONTINUE;
			break;
		case 'b':
			if (str_cmp(str, "break", len, 5))
				return TOK_KEY_BREAK;
			break;
		case 'r':
			if (str_cmp(str, "return", len, 6))
				return TOK_KEY_RETURN;
			if (str_cmp(str, "register", len, 8))
				return TOK_KEY_REGISTER;
			break;
		case 'd':
			if (str_cmp(str, "do", len, 2))
				return TOK_KEY_DO;
			if (str_cmp(str, "double", len, 6))
				return TOK_KEY_DOUBLE;
			if (str_cmp(str, "default", len, 7))
				return TOK_KEY_DEFAULT;
			break;
		case 'l':
			if (str_cmp(str, "long", len, 4))
				return TOK_KEY_LONG;
			break;
		case 'u':
			if (str_cmp(str, "union", len, 5))
				return TOK_KEY_UNION;
			if (str_cmp(str, "unsigned", len, 8))
				return TOK_KEY_UNSIGNED;
			break;
		case 't':
			if (str_cmp(str, "typedef", len, 7))
				return TOK_KEY_TYPEDEF;
			break;
		case 'v':
			if (str_cmp(str, "void", len, 4))
				return TOK_KEY_VOID;
			if (str_cmp(str, "volatile", len, 8))
				return TOK_KEY_VOLATILE;
			break;
		case 'g':
			if (str_cmp(str, "goto", len, 4))
				return TOK_KEY_GOTO;
			break;
		default:
			return TOK_IDENTIFIER;
	}
	
	return TOK_IDENTIFIER;
}


static enum lex_token_type
lookup_symbol(const char* str, uint32_t* symbol_len)
{
	switch (*str) {
		case '+':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_OP_PLUS_ASSIGN;
			} else if (str[1] == '+') {
				*symbol_len = 2;
				return TOK_OP_INCREASE;
			}
			*symbol_len = 1;
			return TOK_OP_PLUS;
		case '-':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_OP_MINUS_ASSIGN;
			} else if (str[1] == '-') {
				*symbol_len = 2;
				return TOK_OP_DECREASE;
			} else if (str[1] == '>') {
				*symbol_len = 2;
				return TOK_CTRL_ARROW;
			}
			*symbol_len = 1;
			return TOK_OP_MINUS;
		case '/':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_OP_DIV_ASSIGN;
			}
			*symbol_len = 1;
			return TOK_OP_DIV;
		case '*':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_OP_STAR_ASSIGN;
			}
			*symbol_len = 1;
			return TOK_OP_STAR;
		case '%':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_OP_MOD_ASSIGN;
			}
			*symbol_len = 1;
			return TOK_OP_MOD;
		case '=':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_LOG_EQUAL;
			}
			*symbol_len = 1;
			return TOK_OP_ASSIGN;
		case '&':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_BIT_AMPERSAND_ASSIGN;
			} else if (str[1] == '&') {
				*symbol_len = 2;
				return TOK_LOG_AND;
			}
			*symbol_len = 1;
			return TOK_BIT_AMPERSAND;
		case '|':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_BIT_OR_ASSIGN;
			} else if (str[1] == '|') {
				*symbol_len = 2;
				return TOK_LOG_OR;
			}
			*symbol_len = 1;
			return TOK_BIT_OR;
		case '~':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_BIT_NOT_ASSIGN;
			}
			*symbol_len = 1;
			return TOK_BIT_NOT;
		case '^':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_BIT_EOR_ASSIGN;
			}
			*symbol_len = 1;
			return TOK_BIT_EOR;
		case '>':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_LOG_GREATER_OR_EQUAL;
			} else if (str[1] == '>') {
				if (str[2] == '=') {
					*symbol_len = 3;
					return TOK_BIT_RIGHT_SHIFT_ASSIGN;
				} else {
					*symbol_len = 2;
					return TOK_BIT_RIGHT_SHIFT;
				}
			}
			*symbol_len = 1;
			return TOK_LOG_GREATER;
		case '<':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_LOG_LESSER_OR_EQUAL;
			} else if (str[1] == '<') {
				if (str[2] == '=') {
					*symbol_len = 3;
					return TOK_BIT_LEFT_SHIFT_ASSIGN;
				} else {
					*symbol_len = 2;
					return TOK_BIT_LEFT_SHIFT;
				}
			}
			*symbol_len = 1;
			return TOK_LOG_LESSER;
		case '(':
			*symbol_len = 1;
			return TOK_CTRL_PAREN_OPEN;
		case ')':
			*symbol_len = 1;
			return TOK_CTRL_PAREN_CLOSED;
		case '[':
			*symbol_len = 1;
			return TOK_CTRL_BRACKET_OPEN;
		case ']':
			*symbol_len = 1;
			return TOK_CTRL_BRACKET_CLOSED;
		case '{':
			*symbol_len = 1;
			return TOK_CTRL_BRACE_OPEN;
		case '}':
			*symbol_len = 1;
			return TOK_CTRL_BRACE_CLOSED;
		case '!':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_LOG_NOT_EQUAL;
			}
			*symbol_len = 1;
			return TOK_LOG_NOT;
		case ',':
			*symbol_len = 1;
			return TOK_CTRL_COMMA;
		case '.':
			if (str[1] == '.') {
				if (str[2] == '.') {
					*symbol_len = 3;
					return TOK_CTRL_ELLIPSIS;
				}
			}
			*symbol_len = 1;
			return TOK_CTRL_DOT;
		case ';':
			*symbol_len = 1;
			return TOK_CTRL_SEMIKOLON;
		case ':':
			*symbol_len = 1;
			return TOK_CTRL_KOLON;
		case '?':
			*symbol_len = 1;
			return TOK_CTRL_QUERY;
		case '\0':
			*symbol_len = 0;
			return TOK_EOF;
	}

	*symbol_len = 1;
	return TOK_UNKNOWN;
}

struct lex_token
lex_next_token(struct lex_file_buffer* file_buffer)
{
	char c;
	uint32_t token_len;
	struct lex_token token;

	c = skip_whitespace_and_comments(file_buffer);

	if (isalpha(c) || c == '_') {
		token_len = word_len(file_buffer);
		token.type = lookup_keyword(file_buffer->curr, token_len);
		skip(file_buffer, token_len);
	}
	// We should check if there is a digit after after a '.'
	else if (isdigit(c)) { 
		token_len = number_len(file_buffer);	
		token.type = parse_decimal_constant(file_buffer->curr, 
				token_len, &token.value_int);
		skip(file_buffer, token_len);
	}
	else if (c == '"') {
		token.type = TOK_CONSTANT_STRING;
		next(file_buffer);
		token_len = string_len(file_buffer);
		skip(file_buffer, token_len + 1);
	}
	else if (c == '\'') {
		token.type = TOK_CONSTANT_CHAR;
		next(file_buffer);
		token_len = char_len(file_buffer);
		token.value_int = parse_char_literal(file_buffer, token_len);
		skip(file_buffer, token_len + 1);
	}
	else {
		token.type = lookup_symbol(file_buffer->curr, &token_len);
		skip(file_buffer, token_len);
	}

	return token;
}
