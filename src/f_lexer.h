#ifndef _F_LEXER_
#define _F_LEXER_

#include "f_def.h"
#include "type.h"
#include "symbol.h"
#include "err.h"

#include <stdint.h>

typedef enum lex_token_type
{
	/* special */
	TOK_NULL,					// used to indicate an array of types and such
	TOK_UNKNOWN,				// £

	/* constants */
	TOK_LITERAL,				// 1234 or 1.234
	
	/* operators */
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
	
	/* logical operators */
	TOK_GREATER,				// >
	TOK_LESSER,					// <
	TOK_GREATER_OR_EQUAL,		// >=
	TOK_LESSER_OR_EQUAL,		// <=
	TOK_EQUAL,					// ==
	TOK_NOT_EQUAL,				// !=
	TOK_NOT,					// !
	TOK_AND,					// &&
	TOK_OR,						// ||

	/* bitwise operators */
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

	/* ctrl operands */
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

	/* keywords */
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

} lex_token_type_t;


typedef struct lex_token
{
	lex_token_type_t			type;
	err_location_t				err_loc;
	
	union
	{
		literal_t				literal;
		sym_hash_t				hash;
	};

} lex_token_t;

typedef struct lexer
{
	uint32_t					line;
	uint32_t					col;

	char*						filename;
	
	char*						start;
	char*						curr;
	char*						end;

	lex_token_t					last_token;
	lex_token_t					curr_token;
	lex_token_t					next_token;

} lexer_t;

err_location_t		err_loc(const lexer_t* lexer);
const char*			lex_tok_debug_str(lex_token_type_t type);

void				f_create_lexer(lexer_t *lexer, const char* filename);
void				f_destroy_lexer(lexer_t *lexer);

lex_token_t			f_next_token(lexer_t *lexer);


#endif
