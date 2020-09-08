#include "def.h"
#include "sym.h"
#include "lex.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

struct err_location
err_loc(const struct lex_file_buffer *fb)
{
	struct err_location loc = { 
		.line	  = fb->line,
		.col	  = fb->col,
		.filename = fb->filename
	};

	return loc;
}

static const char *tok_str[_TOK_COUNT + 1] = {
	"TOK_UNKNOWN",
	"TOK_CONSTANT_INT",
	"TOK_CONSTANT_FLOAT",
	"TOK_CONSTANT_STRING",
	"TOK_CONSTANT_CHAR",
	"TOK_PLUS",
	"TOK_MINUS",
	"TOK_STAR",
	"TOK_DIV",
	"TOK_MOD",
	"TOK_PLUS_ASSIGN",
	"TOK_MINUS_ASSIGN",
	"TOK_STAR_ASSIGN",
	"TOK_DIV_ASSIGN",
	"TOK_MOD_ASSIGN",
	"TOK_INCREASE",
	"TOK_DECREASE",
	"TOK_ASSIGN",
	"TOK_GREATER",
	"TOK_LESSER",
	"TOK_GREATER_OR_EQUAL",
	"TOK_LESSER_OR_EQUAL",
	"TOK_EQUAL",
	"TOK_NOT_EQUAL",
	"TOK_NOT",
	"TOK_AND",
	"TOK_OR",
	"TOK_AMPERSAND",
	"TOK_NOT_BIT",
	"TOK_OR_BIT",
	"TOK_EOR",
	"TOK_LEFT_SHIFT",
	"TOK_RIGHT_SHIFT",
	"TOK_AMPERSAND_ASSIGN",
	"TOK_NOT_ASSIGN",
	"TOK_OR_ASSIGN",
	"TOK_EOR_ASSIGN",
	"TOK_LEFT_SHIFT_ASSIGN",
	"TOK_RIGHT_SHIFT_ASSIGN",
	"TOK_PAREN_OPEN",
	"TOK_PAREN_CLOSED",
	"TOK_BRACKET_OPEN",
	"TOK_BRACKET_CLOSED",
	"TOK_BRACE_OPEN",
	"TOK_BRACE_CLOSED",
	"TOK_DOT",
	"TOK_ARROW",
	"TOK_SEMIKOLON",
	"TOK_KOLON",
	"TOK_COMMA",
	"TOK_ELLIPSIS",
	"TOK_QUERY",
	"TOK_KEY_IF",
	"TOK_KEY_ELSE",
	"TOK_KEY_WHILE",
	"TOK_KEY_DO",
	"TOK_KEY_FOR",
	"TOK_KEY_SWITCH",
	"TOK_KEY_CASE",
	"TOK_KEY_BREAK",
	"TOK_KEY_DEFAULT",
	"TOK_KEY_CONTINUE",
	"TOK_KEY_RETURN",
	"TOK_KEY_GOTO",
	"TOK_KEY_INT",
	"TOK_KEY_FLOAT",
	"TOK_KEY_CHAR",
	"TOK_KEY_DOUBLE",
	"TOK_KEY_LONG",
	"TOK_KEY_SHORT",
	"TOK_KEY_VOID",
	"TOK_KEY_CONST",
	"TOK_KEY_VOLATILE",
	"TOK_KEY_REGISTER",
	"TOK_KEY_SIGNED",
	"TOK_KEY_UNSIGNED",
	"TOK_KEY_STRUCT",
	"TOK_KEY_ENUM",
	"TOK_KEY_UNION",
	"TOK_KEY_EXTERN",
	"TOK_KEY_STATIC",
	"TOK_KEY_SIZEOF",
	"TOK_KEY_TYPEDEF",
	"TOK_IDENTIFIER",
	"TOK_EOF"
};

const char *
lex_tok_str(enum lex_token_type type)
{
	return tok_str[type];
}

void
lex_load_file_buffer(struct lex_file_buffer *fb, const char *filename)
{
	unsigned int filename_size;
	unsigned int file_size;
	FILE *file = fopen(filename, "r");

	if (!file) {
		fatal_error("%s: No such file", filename);
	}

	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	rewind(file);

	fb->start = c_malloc(file_size);
	fb->curr = fb->start;
	fb->end	= fb->start + file_size;

	filename_size = strlen(filename);
	fb->filename  = c_malloc(filename_size);
	strncpy(fb->filename, filename, filename_size);

	fb->line = 1;
	fb->col	 = 1;

	if (!fread(fb->start, 1, file_size, file)) {
		fatal_error("Failure reading %s", filename);
	}

	fclose(file);

	// lex the first node, so next lex_next_token gets the first node
	lex_next_token(fb);	
}

void
lex_cleanup_file_buffer(struct lex_file_buffer *fb)
{
	c_free(fb->start);
	c_free(fb->filename);

	fb->col	  = 0;
	fb->line  = 0;
	fb->start = NULL;
	fb->curr  = NULL;
	fb->end	  = NULL;
}

static char
next(struct lex_file_buffer *fb)
{
	char c;

	if (fb->curr != fb->end) {
		c = *(++fb->curr);
	} else {
		return '\0';
	}

	if (c == '\n') {
		++fb->line;
		fb->col = 1;
	} else {
		++fb->col;
	}

	return c;
}

static void
skip(struct lex_file_buffer *fb, int n)
{
	int i;
	for (i = 0; i < n; ++i) {
		next(fb);
	}
}

static char
skip_single_line_comment(struct lex_file_buffer *fb)
{
	char c = *fb->curr;

	while (c != '\n' && c != '\0')
		c = next(fb);

	c = next(fb);

	return c;
}

static char
skip_multi_line_comment(struct lex_file_buffer *fb)
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

	syntax_error(err_loc(fb), "Unexpected end of file in comment");
	return '\0';
}

static char
skip_whitespace_and_comments(struct lex_file_buffer *fb)
{
	for (;;) {
		switch (*fb->curr) {
			case ' ':
			case '\n':
			case '\t':
			case '\r':
			case '\v':
			case '\f':
				next(fb);
				break;

			case '/':
				if (fb->curr[1] == '*') {
					skip_multi_line_comment(fb);
				} else if (fb->curr[1] == '/') {
					skip_single_line_comment(fb);
				} else {
					return *fb->curr;
				}
				break;

			default: return *fb->curr;
		}
	}
}

static int
word_len(const struct lex_file_buffer *fb)
{
	int i			= 0;
	const char *str = fb->curr;

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
number_len(const struct lex_file_buffer *fb)
{
	int i			= 0;
	const char *str = fb->curr;

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
string_len(const struct lex_file_buffer *fb)
{
	int i			= 0;
	const char *str = fb->curr;

	while (str[i]) {
		if (str[i] != '"')
			++i;
		else
			return i;
	}

	syntax_error(err_loc(fb), "Unexpected end of file inside string");
	return 0;
}

static int
char_len(const struct lex_file_buffer *fb)
{
	int i = 0, slashes = 0;
	const char *str = fb->curr;

	while (str[i]) {
		if (str[i] == '\'') {
			if (slashes == 1) {
				slashes = 0;
				++i;
				continue;
			}

			return i;
		} else if (str[i] == '\\') {
			++slashes;
			++i;
		} else {
			slashes = 0;
			++i;
		}
	}

	syntax_error(err_loc(fb), "Unexpected end of file inside char literal");

	return 0;
}

static enum lex_token_type
parse_decimal_constant(const char *str, int len, int64_t *val)
{
	int i		 = 0;
	int64_t tmp	 = 0;
	double d_tmp = 0, k = 0.1;

	while (i != len && str[i] != '.') {
		tmp = tmp * 10 + str[i] - '0';
		++i;
	}

	if (str[i] != '.') {
		*val = tmp;
		return TOK_INT_LIT;
	}

	++i;
	d_tmp = (double)tmp;

	while (i != len) {
		d_tmp += (str[i] - '0') * k;
		k *= 0.1;
		++i;
	}

	*((double *)val) = d_tmp;
	return TOK_FLOAT_LIT;
}

static enum lex_token_type
parse_octal_constant(const char *str, int len, int64_t *val)
{
	int i		 = 0;
	int64_t tmp	 = 0;
	double d_tmp = 0, k = 0.125;

	while (i != len && str[i] != '.') {
		tmp = tmp * 8 + str[i] - '0';
		++i;
	}

	if (str[i] != '.') {
		*val = tmp;
		return TOK_INT_LIT;
	}

	++i;
	d_tmp = (double)tmp;

	while (i != len) {
		d_tmp += (str[i] - '0') * k;
		k *= 0.125;
		++i;
	}

	*((double *)val) = d_tmp;
	return TOK_FLOAT_LIT;
}

static enum lex_token_type
parse_hex_constant(const char *str, int len, int64_t *val)
{
	int i		= 0;
	int64_t tmp = 0;

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

	return TOK_INT_LIT;
}

static void
parse_string_literal(const char *str, int len, char *buffer)
{
}

static int
parse_multichar_constant(const struct lex_file_buffer *fb, int len)
{
	syntax_warning(err_loc(fb), "multi-character constant");

	if (len >= 6)
		syntax_warning(err_loc(fb), "multi-character constant is too long");

	return fb->curr[len - 1];
}

static int
parse_char_literal(const struct lex_file_buffer *fb, int len)
{
	int64_t c		= 0;
	const char *str = fb->curr;

	switch (len) {
		case 1: return str[0];
		case 2:
			if (str[0] == '\\') {
				switch (str[1]) {
					case 'a': return '\a';
					case 'b': return '\b';
					case 'f': return '\f';
					case 'r': return '\r';
					case 'n': return '\n';
					case 't': return '\t';
					case 'v': return '\v';
					case '\\': return '\\';
					case '\'': return '\'';

					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
						parse_octal_constant(str + 1, len - 1, &c);
						return c;
					case 'x':
						parse_hex_constant(str + 2, len - 2, &c);
						return c;
					default:
						syntax_error(err_loc(fb), "Invalid escape sequence");
				}
			} else
				return parse_multichar_constant(fb, len);
			break;

		case 3:
		case 4:
			if (str[0] == '\\') {
				switch (str[1]) {
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
						parse_octal_constant(str + 1, len - 1, &c);
						return c;
					case 'x':
						parse_hex_constant(str + 2, len - 2, &c);
						return c;
					case 'u':
						syntax_error(err_loc(fb),
									 "Unicode is yet to be implemented");
						return 0;
					default:
						syntax_error(err_loc(fb), "Invalid escape sequence");
						return 0;
				}
			} else
				return parse_multichar_constant(fb, len);
			break;

		case 8:
			if (str[0] == '\\') {
				if (str[1] == 'U') {
					syntax_error(err_loc(fb),
								 "Unicode is yet to be implemented");
				}
			} else {
				return parse_multichar_constant(fb, len);
			}
			break;

		default: return parse_multichar_constant(fb, len);
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
lookup_symbol(const char *str, int *symbol_len)
{
	switch (*str) {
		case '+':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_PLUS_ASSIGN;
			} else if (str[1] == '+') {
				*symbol_len = 2;
				return TOK_INCREASE;
			}
			*symbol_len = 1;
			return TOK_PLUS;
		case '-':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_MINUS_ASSIGN;
			} else if (str[1] == '-') {
				*symbol_len = 2;
				return TOK_DECREASE;
			} else if (str[1] == '>') {
				*symbol_len = 2;
				return TOK_ARROW;
			}
			*symbol_len = 1;
			return TOK_MINUS;
		case '/':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_DIV_ASSIGN;
			}
			*symbol_len = 1;
			return TOK_DIV;
		case '*':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_STAR_ASSIGN;
			}
			*symbol_len = 1;
			return TOK_STAR;
		case '%':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_MOD_ASSIGN;
			}
			*symbol_len = 1;
			return TOK_MOD;
		case '=':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_EQUAL;
			}
			*symbol_len = 1;
			return TOK_ASSIGN;
		case '&':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_AMPERSAND_ASSIGN;
			} else if (str[1] == '&') {
				*symbol_len = 2;
				return TOK_AND;
			}
			*symbol_len = 1;
			return TOK_AMPERSAND;
		case '|':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_OR_ASSIGN;
			} else if (str[1] == '|') {
				*symbol_len = 2;
				return TOK_OR;
			}
			*symbol_len = 1;
			return TOK_OR_BIT;
		case '~':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_NOT_ASSIGN;
			}
			*symbol_len = 1;
			return TOK_NOT_BIT;
		case '^':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_EOR_ASSIGN;
			}
			*symbol_len = 1;
			return TOK_EOR;
		case '>':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_GREATER_OR_EQUAL;
			} else if (str[1] == '>') {
				if (str[2] == '=') {
					*symbol_len = 3;
					return TOK_RIGHT_SHIFT_ASSIGN;
				} else {
					*symbol_len = 2;
					return TOK_RIGHT_SHIFT;
				}
			}
			*symbol_len = 1;
			return TOK_GREATER;
		case '<':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_LESSER_OR_EQUAL;
			} else if (str[1] == '<') {
				if (str[2] == '=') {
					*symbol_len = 3;
					return TOK_LEFT_SHIFT_ASSIGN;
				} else {
					*symbol_len = 2;
					return TOK_LEFT_SHIFT;
				}
			}
			*symbol_len = 1;
			return TOK_LESSER;
		case '(': *symbol_len = 1; return TOK_PAREN_OPEN;
		case ')': *symbol_len = 1; return TOK_PAREN_CLOSED;
		case '[': *symbol_len = 1; return TOK_BRACKET_OPEN;
		case ']': *symbol_len = 1; return TOK_BRACKET_CLOSED;
		case '{': *symbol_len = 1; return TOK_BRACE_OPEN;
		case '}': *symbol_len = 1; return TOK_BRACE_CLOSED;
		case '!':
			if (str[1] == '=') {
				*symbol_len = 2;
				return TOK_NOT_EQUAL;
			}
			*symbol_len = 1;
			return TOK_NOT;
		case ',': *symbol_len = 1; return TOK_COMMA;
		case '.':
			if (str[1] == '.') {
				if (str[2] == '.') {
					*symbol_len = 3;
					return TOK_ELLIPSIS;
				}
			}
			*symbol_len = 1;
			return TOK_DOT;
		case ';': *symbol_len = 1; return TOK_SEMIKOLON;
		case ':': *symbol_len = 1; return TOK_KOLON;
		case '?': *symbol_len = 1; return TOK_QUERY;
		case '\0': *symbol_len = 0; return TOK_EOF;
	}

	*symbol_len = 1;
	return TOK_UNKNOWN;
}

struct lex_token
lex_next_token(struct lex_file_buffer *fb)
{
	char c;
	int token_len;

	c = skip_whitespace_and_comments(fb);

	fb->last_token = fb->curr_token;
	fb->curr_token = fb->next_token;

	if (isalpha(c) || c == '_') {
		token_len = word_len(fb);
		fb->next_token.hash = sym_hash(fb->curr, token_len);
		fb->next_token.type = lookup_keyword(fb->next_token.hash, token_len);
		skip(fb, token_len);
	}
	else if (isdigit(c) || (c == '.' && isdigit(fb->curr[1]))) {
		token_len = number_len(fb);
		fb->next_token.type = parse_decimal_constant(
			fb->curr, token_len, &fb->curr_token.value.val_int);
		skip(fb, token_len);
	}
	else if (c == '"') {
		fb->next_token.type = TOK_STRING_LIT;
		next(fb);
		token_len = string_len(fb);
		skip(fb, token_len + 1);
	}
	else if (c == '\'') {
		fb->next_token.type = TOK_CHAR_LIT;
		next(fb);
		token_len = char_len(fb);
		fb->next_token.value.val_int = parse_char_literal(fb, token_len);
		skip(fb, token_len + 1);
	}
	else {
		fb->next_token.type = lookup_symbol(fb->curr, &token_len);
		skip(fb, token_len);
	}

	fb->next_token.err_loc = err_loc(fb);
	return fb->curr_token;
}
