#ifndef LEX_H
#define LEX_H

#include "def.h"

enum lex_token_type
{
	// special
	TOK_UNKNOWN,				// £

	// constants
	TOK_INT_LIT,				// 1234
	TOK_FLOAT_LIT,				// 1.234
	TOK_STRING_LIT,				// "1234"
	TOK_CHAR_LIT,				// '1'
	
	// operators
	TOK_PLUS,					// +
	TOK_MINUS,					// -
	TOK_STAR,					// *
	TOK_DIV,					// /
	TOK_MOD,					// %

	TOK_PLUS_ASSIGN,			// +=
	TOK_MINUS_ASSIGN,			// -=
	TOK_STAR_ASSIGN,			// *=
	TOK_DIV_ASSIGN,				// /=
	TOK_MOD_ASSIGN,				// %=

	TOK_INCREASE,				// ++
	TOK_DECREASE,				// --
	TOK_ASSIGN,					// =
	
	// logical operators
	TOK_GREATER,				// >
	TOK_LESSER,					// <
	TOK_GREATER_OR_EQUAL,		// >=
	TOK_LESSER_OR_EQUAL,		// <=
	TOK_EQUAL,					// ==
	TOK_NOT_EQUAL,				// !=
	TOK_NOT,					// !
	TOK_AND,					// &&
	TOK_OR,						// ||

	// bitwise operators
	TOK_AMPERSAND,				// &
	TOK_NOT_BIT,				// ~
	TOK_OR_BIT,					// |
	TOK_EOR,					// ^
	TOK_LEFT_SHIFT,				// <<
	TOK_RIGHT_SHIFT,			// >>
	
	TOK_AMPERSAND_ASSIGN,		// &=
	TOK_NOT_ASSIGN,				// ~=
	TOK_OR_ASSIGN,				// |=
	TOK_EOR_ASSIGN,				// ^=
	TOK_LEFT_SHIFT_ASSIGN,		// <<=
	TOK_RIGHT_SHIFT_ASSIGN,		// >>=

	// ctrl operands
	TOK_PAREN_OPEN,				// (
	TOK_PAREN_CLOSED,			// )
	TOK_BRACKET_OPEN,			// [
	TOK_BRACKET_CLOSED,			// ]
	TOK_BRACE_OPEN,				// {
	TOK_BRACE_CLOSED,			// }
	
	TOK_DOT,					// .
	TOK_ARROW,					// ->
	TOK_SEMIKOLON,				// ;
	TOK_KOLON,					// :
	TOK_COMMA,					// ,
	TOK_ELLIPSIS,				// ...
	TOK_QUERY,					// ?

	// keywords
	TOK_KEY_IF,
	TOK_KEY_ELSE,
	TOK_KEY_WHILE,
	TOK_KEY_DO,
	TOK_KEY_FOR,
	TOK_KEY_SWITCH,
	TOK_KEY_CASE,
	TOK_KEY_BREAK,
	TOK_KEY_DEFAULT,
	TOK_KEY_CONTINUE,
	TOK_KEY_RETURN,
	TOK_KEY_GOTO,
	TOK_KEY_INT,
	TOK_KEY_FLOAT,
	TOK_KEY_CHAR,
	TOK_KEY_DOUBLE,
	TOK_KEY_LONG,
	TOK_KEY_SHORT,
	TOK_KEY_VOID,
	TOK_KEY_CONST,
	TOK_KEY_VOLATILE,
	TOK_KEY_REGISTER,
	TOK_KEY_SIGNED,
	TOK_KEY_UNSIGNED,
	TOK_KEY_STRUCT,
	TOK_KEY_ENUM,
	TOK_KEY_UNION,
	TOK_KEY_EXTERN,
	TOK_KEY_STATIC,
	TOK_KEY_SIZEOF,
	TOK_KEY_TYPEDEF,

	TOK_IDENTIFIER,
	TOK_EOF,
	_TOK_COUNT
};

struct lex_token
{
	enum lex_token_type		type;
	struct err_location		err_loc;
	
	union
	{
		union const_value	value;
		int64_t				hash;
	};
};

struct lex_file_buffer
{
	int						line;
	int						col;

	char*					filename;
	
	char*					start;
	char*					curr;
	char*					end;

	struct lex_token		last_token;
	struct lex_token		curr_token;
	struct lex_token		next_token;
};

struct err_location
err_loc(const struct lex_file_buffer* fb);

const char*
lex_tok_str(enum lex_token_type type);

void
lex_load_file_buffer(struct lex_file_buffer* file_buffer, const char* filename);

void
lex_cleanup_file_buffer(struct lex_file_buffer* file_buffer);

struct lex_token
lex_next_token(struct lex_file_buffer* fb);

#endif
