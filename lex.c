#include "def.h"
#include "sym.h"
#include "lex.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

void
lex_load_file_buffer(struct lex_file_buffer* fb, const char* filename)
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

	fb->start = (char*)c_malloc(file_size);
	fb->curr = fb->start;
	fb->end = fb->start + file_size;

	filename_size = strlen(filename);
	fb->filename = (char*)c_malloc(filename_size);
	strncpy(fb->filename, filename, filename_size);

	fb->line = 0;
	fb->col = 0;

	if (!fread(fb->start, 1, file_size, file)) {
		fatal_error("Failure reading %s", filename);
	}

	fclose(file);
}

void
lex_cleanup_file_buffer(struct lex_file_buffer* fb)
{
	c_free(fb->start);
	c_free(fb->filename);

	fb->col		= 0;
	fb->line	= 0;
	fb->start	= NULL;
	fb->curr	= NULL;
	fb->end		= NULL;
}

static struct err_location
err_loc(const struct lex_file_buffer* fb)
{
	struct err_location loc = {
		.line		= fb->line,
		.col		= fb->col,
		.filename	= fb->filename
	};

	return loc;
}

static char
next(struct lex_file_buffer* fb)
{
	char c;

	if (fb->curr != fb->end) {
		c = *(++fb->curr);
	}
	else {
		return '\0';
	}

	if (c == '\n') {
		++fb->line;
		fb->col = 0;
	}
	else {
		++fb->col;
	}

	return c;
}

static void
skip(struct lex_file_buffer* fb, int n)
{
	int i;
	for (i = 0; i < n; ++i) {
		next(fb);
	}
}

static char
skip_single_line_comment(struct lex_file_buffer* fb)
{
	char c = *fb->curr;

	while (c != '\n' && c != '\0')
		c = next(fb);

	c = next(fb);

	return c;
}

static char
skip_multi_line_comment(struct lex_file_buffer* fb)
{
	// skip opening /*
	skip(fb, 2);

	while (fb->curr != fb->end) {
		if (fb->curr[0] == '*') {
			if (fb->curr[1] == '/') {
				skip(fb, 2);
				return *fb->curr;
			}
		}
		next(fb);
	}

	syntax_error(err_loc(fb),
			"Unexpected end of file in comment");
	return '\0';
}

static char
skip_whitespace_and_comments(struct lex_file_buffer* fb)
{
	for (;;) {
		switch (*fb->curr) {
			case ' ': case '\n': case '\t':
			case '\r': case '\v': case '\f':
				next(fb);
				break;

			case '/':
				if (fb->curr[1] == '*') {
					skip_multi_line_comment(fb);
				}
				else if (fb->curr[1] == '/') {
					skip_single_line_comment(fb);
				}
				else {
					return *fb->curr;
				}
				break;

			default:
				return *fb->curr;
		}
	}
}

static int
word_len(const struct lex_file_buffer* fb)
{
	int i = 0;
	const char* str = fb->curr;
	
	while (str[i]) {
		if (isalpha(str[i]) || str[i] == '_' || isdigit(str[i]))
			++i;
		else
			return i;
	}

	syntax_error(err_loc(fb), "Unexpected end of file");
	return 0;
}

static int
number_len(const struct lex_file_buffer* fb)
{
	int i = 0;
	const char* str = fb->curr;

	while (str[i]) {
		if (isdigit(str[i]) || str[i] == '.')
			++i;
		else
			return i;
	}

	syntax_error(err_loc(fb), "Unexpected end of file");
	return 0;
}

static int
string_len(const struct lex_file_buffer* fb)
{
	int i = 0;
	const char* str = fb->curr;

	while (str[i]) {
		if (str[i] != '"')
			++i;
		else
			return i;
	}

	syntax_error(err_loc(fb),
			"Unexpected end of file inside string");
	return 0;
}

static int
char_len(const struct lex_file_buffer* fb)
{
	int i = 0, slashes = 0;
	const char* str = fb->curr;

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

	syntax_error(err_loc(fb), 
			"Unexpected end of file inside char literal");

	return 0;
}

static enum lex_token_type
parse_decimal_constant(const char* str, int len, int64_t* val)
{
	int i = 0;
	int64_t tmp = 0;
	double d_tmp = 0, k = 0.1;

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
parse_octal_constant(const char* str, int len, int64_t* val)
{
	int i = 0;
	int64_t tmp = 0;
	double d_tmp = 0, k = 0.125;

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
parse_hex_constant(const char* str, int len, int64_t* val)
{
	int i = 0;
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
parse_string_literal(const char* str, int len, char* buffer)
{
	
}

static int
parse_multichar_constant(const struct lex_file_buffer* fb, int len)
{
	syntax_warning(err_loc(fb),
		"multi-character constant");

	if (len >= 6)
		syntax_warning(err_loc(fb),
			"multi-character constant is too long");

	return fb->curr[len - 1];
}

static int
parse_char_literal(const struct lex_file_buffer* fb, int len)
{
	int64_t c = 0;
	const char* str = fb->curr;

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
						syntax_error(err_loc(fb), "Invalid escape sequence");
				}
			}
			else
				return parse_multichar_constant(fb, len);		
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
						syntax_error(err_loc(fb), "Unicode is yet to be implemented");
						return 0;
					default:
						syntax_error(err_loc(fb), "Invalid escape sequence");
						return 0;
				}
			}
			else
				return parse_multichar_constant(fb, len);
			break;

		case 8:
			if (str[0] == '\\') {
				if (str[1] == 'U') {
					syntax_error(err_loc(fb),
							"Unicode is yet to be implemented");
				}
			}
			else {
				return parse_multichar_constant(fb, len);
			}
			break;

		default:
			return parse_multichar_constant(fb, len);
	}
	
	assert(false);
}

static enum lex_token_type
lookup_keyword(int64_t hash, int len)
{
	switch (len) {
		case 2:
			if (hash == 210805918999320793)
				return TOK_KEY_IF;
			if (hash == 3416275755843533644)
				return TOK_KEY_DO;
			break;

		case 3:
			if (hash == 7858283700166231528)
				return TOK_KEY_INT;
			if (hash == 691152041897930411)
				return TOK_KEY_FOR;
			break;

		case 4:
			if (hash == 611863941455303809)
				return TOK_KEY_ELSE;
			if (hash == 1427961834693360368)
				return TOK_KEY_ENUM;
			if (hash == 4089114412274189689)
				return TOK_KEY_CHAR;
			if (hash == 1765111298423762854)
				return TOK_KEY_CASE;
			if (hash == 4396619338160439502)
				return TOK_KEY_LONG;
			if (hash == 7255839161879393319)
				return TOK_KEY_VOID;
			if (hash == 8715651464562445227)
				return TOK_KEY_GOTO;
			break;

		case 5:
			if (hash == 3875119015612757251)
				return TOK_KEY_WHILE;
			if (hash == 2807200987411122317)
				return TOK_KEY_FLOAT;
			if (hash == 1402861911904202446)
				return TOK_KEY_SHORT;
			if (hash == 5471028013491099405)
				return TOK_KEY_CONST;
			if (hash == 9152775002878652110)
				return TOK_KEY_BREAK;
			if (hash == 270728114491518239)
				return TOK_KEY_UNION;
			break;

		case 6:
			if (hash == 7727572673835008135)
				return TOK_KEY_EXTERN;
			if (hash == 2646967638904444428)
				return TOK_KEY_SWITCH;
			if (hash == 5693220595859545018)
				return TOK_KEY_STRUCT;
			if (hash == 7489007233736309326)
				return TOK_KEY_STATIC;
			if (hash == 884287416668004567)
				return TOK_KEY_SIZEOF;
			if (hash == 3510987647836465774)
				return TOK_KEY_SIGNED;
			if (hash == 3362893395776512250)
				return TOK_KEY_RETURN;
			if (hash == 7342538386031234340)
				return TOK_KEY_DOUBLE;
			break;

		case 7:
			if (hash == 2936402720817092402)
				return TOK_KEY_DEFAULT;
			if (hash == 3794314392136692499)
				return TOK_KEY_TYPEDEF;
			break;

		case 8:
			if (hash == 3631951867840661473)
				return TOK_KEY_CONTINUE;
			if (hash == 7202289517847671133)
				return TOK_KEY_REGISTER;
			if (hash == 2298029417117381043)
				return TOK_KEY_UNSIGNED;
			if (hash == 8161154796913793944)
				return TOK_KEY_VOLATILE;
			break;
	}

	return TOK_IDENTIFIER;
}

static enum lex_token_type
lookup_symbol(const char* str, int* symbol_len)
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
lex_next_token(struct lex_file_buffer* fb, struct sym_table* table)
{
	char c;
	int token_len;
	int64_t hash;
	struct sym_entry id_entry;

	c = skip_whitespace_and_comments(fb);

	if (isalpha(c) || c == '_') {
		token_len = word_len(fb);
		hash = sym_hash(fb->curr, token_len);
		fb->last.type = lookup_keyword(hash, token_len);
		skip(fb, token_len);

		if (fb->last.type == TOK_IDENTIFIER) {
			id_entry.hash = hash;
			fb->last.value.id = sym_find_entry(table, hash);	

			if (fb->last.value.id != -1)
				fb->last.value.id = sym_add_entry(table, id_entry);
		}
	}
	// We should check if there is a digit after after a '.'
	else if (isdigit(c)) { 
		token_len = number_len(fb);	
		fb->last.type = parse_decimal_constant(fb->curr, 
				token_len, &fb->last.value.val_int);
		skip(fb, token_len);
	}
	else if (c == '"') {
		fb->last.type = TOK_CONSTANT_STRING;
		next(fb);
		token_len = string_len(fb);
		skip(fb, token_len + 1);
	}
	else if (c == '\'') {
		fb->last.type = TOK_CONSTANT_CHAR;
		next(fb);
		token_len = char_len(fb);
		fb->last.value.val_int = parse_char_literal(fb, token_len);
		skip(fb, token_len + 1);
	}
	else {
		fb->last.type = lookup_symbol(fb->curr, &token_len);
		skip(fb, token_len);
	}

	fb->last.err_loc = err_loc(fb);
	return fb->last;
}
