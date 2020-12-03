#include "def.h"
#include "sym.h"
#include "lex.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*
 * parses the raw text into tokens
 */

static const char *tok_str[_TOK_COUNT] = {
	"TOK_UNKNOWN",
	"TOK_LIT",
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

const char*
lex_tok_str(enum lex_token_type type)
{
	return tok_str[type];
}

static char
next(struct lex_instance *in)
{
	char c;

	if (in->curr != in->end) {
		c = *(++in->curr);
	} else {
		return '\0';
	}

	if (c == '\n') {
		++in->line;
		in->col = 1;
	} else {
		++in->col;
	}

	return c;
}

static void
skip(struct lex_instance *in, uint32_t n)
{
	uint32_t i;
	for (i = 0; i < n; ++i) {
		next(in);
	}
}

static char
skip_single_line_comment(struct lex_instance *in)
{
	char c = *in->curr;

	while (c != '\n' && c != '\0')
		c = next(in);

	c = next(in);

	return c;
}

static char
skip_multi_line_comment(struct lex_instance *in)
{
	// skip opening /*
	skip(in, 2);

	while (in->curr != in->end) {
		if (in->curr[0] == '*') {
			if (in->curr[1] == '/') {
				skip(in, 2);
				return *in->curr;
			}
		}
		next(in);
	}

	syntax_error(err_loc(in), "Unexpected end of file in comment");
	return '\0';
}

static char
skip_whitespace_and_comments(struct lex_instance *in)
{
	for (;;) {
		switch (*in->curr) {
			case ' ':
			case '\n':
			case '\t':
			case '\r':
			case '\v':
			case '\f':
				next(in);
				break;

			case '/':
				if (in->curr[1] == '*') {
					skip_multi_line_comment(in);
				} else if (in->curr[1] == '/') {
					skip_single_line_comment(in);
				} else {
					return *in->curr;
				}
				break;

			default: return *in->curr;
		}
	}
}

static int
word_len(const struct lex_instance *in)
{
	uint32_t i = 0;
	const char *str = in->curr;

	while (str[i]) {
		if (isalpha(str[i]) || str[i] == '_' || isdigit(str[i]))
			++i;
		else
			return i;
	}

	syntax_error(err_loc(in), "Unexpected end of file");
	return 0;
}

static int
number_len(const struct lex_instance *in)
{
	uint32_t i = 0;
	const char *str = in->curr;

	while (str[i]) {
		if (isdigit(str[i]) || str[i] == '.')
			++i;
		else
			return i;
	}

	syntax_error(err_loc(in), "Unexpected end of file");
	return 0;
}

static int
string_len(const struct lex_instance *in)
{
	uint32_t i = 0;
	const char *str = in->curr;

	while (str[i]) {
		if (str[i] != '"')
			++i;
		else
			return i;
	}

	syntax_error(err_loc(in), "Unexpected end of file inside string");
	return 0;
}

static int
char_len(const struct lex_instance *in)
{
	uint32_t i = 0, slashes = 0;
	const char *str = in->curr;

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

	syntax_error(err_loc(in), "Unexpected end of file inside char literal");

	return 0;
}

#define MAX_SUFFIX_LEN 2

static struct type_info
parse_suffix(struct lex_instance *lex_in, uint32_t len)
{
	struct type_info type = NULL_TYPE_INFO;

	if (len > MAX_SUFFIX_LEN) {
		syntax_error(err_loc(lex_in), "invalid suffix");
	}

	switch (tolower(*lex_in->curr)) {
		case 'u':
			type.prim = TYPE_PRIM_INT;						
			type.spec &= TYPE_SPEC_UNSIGNED;

			if (len == 2) {
				if (tolower(lex_in->curr[1]) == 'l') {
					type.spec &= TYPE_SPEC_LONG;
				} else {
					syntax_error(err_loc(lex_in), "invalid suffix");
				}
			}
			break;

		case 'l':
			type.prim = TYPE_PRIM_INT;
			type.spec &= TYPE_SPEC_LONG;

			if (len == 2) {
				if (tolower(lex_in->curr[1]) == 'u') {
					type.spec &= TYPE_SPEC_UNSIGNED;
				} else {
					syntax_error(err_loc(lex_in), "invalid suffix");
				}
			}
			break;

		case 'f':
			type.prim = TYPE_PRIM_DOUBLE;

			if (len == 2) {
				syntax_error(err_loc(lex_in), "invalid suffix");
			}
			break;

		default:
			syntax_error(err_loc(lex_in), "invalid suffix");
			break;
	}

	return type;
}

static enum type_literal_type
parse_decimal_constant(const char *str, uint32_t len, uint64_t *val)
{
	uint32_t i = 0;
	int64_t tmp	= 0;
	double d_tmp = 0, k = 0.1;

	while (i != len && str[i] != '.') {
		tmp = tmp * 10 + str[i] - '0';
		++i;
	}

	if (str[i] != '.') {
		*val = tmp;
		return TYPE_LIT_INT;
	}

	++i;
	d_tmp = (double)tmp;

	while (i != len) {
		d_tmp += (str[i] - '0') * k;
		k *= 0.1;
		++i;
	}

	*((double *)val) = d_tmp;
	return TYPE_LIT_INT;
}

static enum type_literal_type
parse_octal_constant(const char *str, uint32_t len, uint64_t *val)
{
	uint32_t i = 0;
	int64_t tmp	= 0;
	double d_tmp = 0, k = 0.125;

	while (i != len && str[i] != '.') {
		tmp = tmp * 8 + str[i] - '0';
		++i;
	}

	if (str[i] != '.') {
		*val = tmp;
		return TYPE_LIT_INT;
	}

	++i;
	d_tmp = (double)tmp;

	while (i != len) {
		d_tmp += (str[i] - '0') * k;
		k *= 0.125;
		++i;
	}

	*((double *)val) = d_tmp;
	return TYPE_LIT_FLOAT;
}

static enum type_literal_type
parse_hex_constant(const char *str, uint32_t len, uint64_t *val)
{
	uint32_t i = 0;
	int64_t tmp = 0;

	while (i != len) {
		if (str[i] >= 'a' && str[i] <= 'f') {
			tmp = tmp * 16 + str[i] - 0x57;
		} else if (str[i] >= 'A' && str[i] <= 'F') {
			tmp = tmp * 16 + str[i] - 0x37;
		} else {
			tmp = tmp * 16 + str[i] - '0';
		}
		++i;
	}

	*val = tmp;

	// TODO add floating point hex literals

	return TYPE_LIT_INT;
}

static void
parse_string_literal(const char *str, uint32_t len, char *buffer)
{
}

static int32_t
parse_multichar_constant(const struct lex_instance *in, uint32_t len)
{
	syntax_warning(err_loc(in), "multi-character constant");

	if (len >= 6) {
		syntax_warning(err_loc(in), "multi-character constant is too long");
	}

	return in->curr[len - 1];
}

static int
parse_char_literal(const struct lex_instance *in, uint32_t len)
{
	uint64_t c		= 0;
	const char *str = in->curr;

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
						syntax_error(err_loc(in), "Invalid escape sequence");
				}
			} else
				return parse_multichar_constant(in, len);
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
						syntax_error(err_loc(in),
									 "Unicode is yet to be implemented");
						return 0;
					default:
						syntax_error(err_loc(in), "Invalid escape sequence");
						return 0;
				}
			} else
				return parse_multichar_constant(in, len);
			break;

		case 8:
			if (str[0] == '\\') {
				if (str[1] == 'U') {
					syntax_error(err_loc(in),
								 "Unicode is yet to be implemented");
				}
			} else {
				return parse_multichar_constant(in, len);
			}
			break;

		default:
			return parse_multichar_constant(in, len);
	}

	assert(false);
	return 0;
}

/* lookup table with pre hashed values for keywords */
static enum lex_token_type
lookup_keyword(int64_t hash, uint32_t len)
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
lookup_symbol(const char *str, uint32_t *symbol_len)
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

/* ================================================================================= */

struct err_location
err_loc(const struct lex_instance *lex_in)
{
	struct err_location loc = { 
		.line = lex_in->line,
		.col = lex_in->col,
		.filename = lex_in->filename
	};

	return loc;
}

struct lex_instance
lex_create_instance(const char *filename)
{
	uint32_t filename_size;
	size_t file_size;
	struct lex_instance lex_in;
	FILE *file = fopen(filename, "r");

	if (!file) {
		fatal_error("%s: No such file", filename);
	}

	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	rewind(file);

	lex_in.start = c_malloc(file_size);
	lex_in.curr = lex_in.start;
	lex_in.end	= lex_in.start + file_size;

	filename_size = strlen(filename);
	lex_in.filename  = c_malloc(filename_size);
	strncpy(lex_in.filename, filename, filename_size);

	lex_in.line = 1;
	lex_in.col = 1;

	if (!fread(lex_in.start, 1, file_size, file)) {
		fatal_error("Failure reading %s", filename);
	}

	fclose(file);

	/* lex the first node, so next lex_next_token gets the first node */
	lex_next_token(&lex_in);	
	
	return lex_in;
}

void
lex_destroy_instance(struct lex_instance *in)
{
	c_free(in->start);
	c_free(in->filename);

	in->col	  = 0;
	in->line  = 0;
	in->start = NULL;
	in->curr  = NULL;
	in->end	  = NULL;
}


struct lex_token
lex_next_token(struct lex_instance *in)
{
	char c;
	uint32_t token_len;
	enum type_literal_type lit_type;
	struct type_info suffix_type;

	c = skip_whitespace_and_comments(in);

	in->last_token = in->curr_token;
	in->curr_token = in->next_token;

	if (isalpha(c) || c == '_') {
		token_len = word_len(in);
		in->next_token.hash = sym_hash(in->curr, token_len);
		in->next_token.type = lookup_keyword(in->next_token.hash, token_len);
		skip(in, token_len);
	} else if (isdigit(c) || (c == '.' && isdigit(in->curr[1]))) {
		in->next_token.type = TOK_LITERAL;

		token_len = number_len(in);

		lit_type = parse_decimal_constant(in->curr, token_len,
			&in->next_token.literal.value.val_int);
		
		/* set type */
		in->next_token.literal.type = type_deduct_from_literal(
			in->next_token.literal.value,
			lit_type
		);

		/* skip the number */
		skip(in, token_len);

		/* parse suffix */				
		token_len = word_len(in);

		if (token_len) {
			suffix_type = parse_suffix(in, token_len);

			/* TODO this should be some kind of error checking */
			assert(type_compat(suffix_type, in->next_token.literal.type) != TYPE_COMPAT_INCOMPAT);

			in->next_token.literal.type = type_combine_suffix_and_lit(suffix_type, in->next_token.literal.type);

			skip(in, token_len);
		}
		
	} else if (c == '"') {
		/* TODO add parsing for strings, and remember deducting the type */
		in->next_token.type = TOK_LITERAL;

		next(in);

		token_len = string_len(in);

		skip(in, token_len + 1);

	} else if (c == '\'') {
		in->next_token.type = TOK_LITERAL;

		next(in);
		token_len = char_len(in);

		/* get value */
		in->next_token.literal.value.val_int = parse_char_literal(in, token_len);

		/* set type */
		in->next_token.literal.type = type_deduct_from_literal(
				in->next_token.literal.value,
				TYPE_LIT_INT
		);

		skip(in, token_len + 1);

	} else {
		in->next_token.type = lookup_symbol(in->curr, &token_len);
		skip(in, token_len);
	}

	in->next_token.err_loc = err_loc(in);
	return in->curr_token;
}
