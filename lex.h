#ifndef LEX_H
#define LEX_H

#include "def.h"

enum lex_token_type
{
	// special
	TOK_UNKNOWN,				// £

	// constants
	TOK_CONSTANT_INT,			// 1234
	TOK_CONSTANT_FLOAT,			// 1.234
	TOK_CONSTANT_STRING,		// "1234"
	TOK_CONSTANT_CHAR,			// '1'
	
	// operators
	TOK_OP_PLUS,				// +
	TOK_OP_MINUS,				// -
	TOK_OP_STAR,				// *
	TOK_OP_DIV,					// /
	TOK_OP_MOD,					// %

	TOK_OP_PLUS_ASSIGN,			// +=
	TOK_OP_MINUS_ASSIGN,		// -=
	TOK_OP_STAR_ASSIGN,			// *=
	TOK_OP_DIV_ASSIGN,			// /=
	TOK_OP_MOD_ASSIGN,			// %=

	TOK_OP_INCREASE,			// ++
	TOK_OP_DECREASE,			// --
	TOK_OP_ASSIGN,				// =
	
	// logical operators
	TOK_LOG_GREATER,			// >
	TOK_LOG_LESSER,				// <
	TOK_LOG_GREATER_OR_EQUAL,	// >=
	TOK_LOG_LESSER_OR_EQUAL,	// <=
	TOK_LOG_EQUAL,				// ==
	TOK_LOG_NOT_EQUAL,			// !=
	TOK_LOG_NOT,				// !
	TOK_LOG_AND,				// &&
	TOK_LOG_OR,					// ||

	// bitwise operators
	TOK_BIT_AMPERSAND,			// &
	TOK_BIT_NOT,				// ~
	TOK_BIT_OR,					// |
	TOK_BIT_EOR,				// ^
	TOK_BIT_LEFT_SHIFT,			// <<
	TOK_BIT_RIGHT_SHIFT,		// >>
	
	TOK_BIT_AMPERSAND_ASSIGN,	// &=
	TOK_BIT_NOT_ASSIGN,			// ~=
	TOK_BIT_OR_ASSIGN,			// |=
	TOK_BIT_EOR_ASSIGN,			// ^=
	TOK_BIT_LEFT_SHIFT_ASSIGN,	// <<=
	TOK_BIT_RIGHT_SHIFT_ASSIGN,	// >>=

	// ctrl operands
	TOK_CTRL_PAREN_OPEN,		// (
	TOK_CTRL_PAREN_CLOSED,		// )
	TOK_CTRL_BRACKET_OPEN,		// [
	TOK_CTRL_BRACKET_CLOSED,	// ]
	TOK_CTRL_BRACE_OPEN,		// {
	TOK_CTRL_BRACE_CLOSED,		// }
	
	TOK_CTRL_DOT,				// .
	TOK_CTRL_ARROW,				// ->
	TOK_CTRL_SEMIKOLON,			// ;
	TOK_CTRL_KOLON,				// :
	TOK_CTRL_COMMA,				// ,
	TOK_CTRL_ELLIPSIS,			// ...
	TOK_CTRL_QUERY,				// ?

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

static const char* lex_tok_to_str[_TOK_COUNT + 1] =
{
	"TOK_UNKNOWN",
	"TOK_CONSTANT_INT",		
	"TOK_CONSTANT_FLOAT",		
	"TOK_CONSTANT_STRING",		
	"TOK_CONSTANT_CHAR",		
	"TOK_OP_PLUS",
	"TOK_OP_MINUS", 
	"TOK_OP_STAR",
	"TOK_OP_DIV",				
	"TOK_OP_MOD",				
	"TOK_OP_PLUS_ASSIGN",		
	"TOK_OP_MINUS_ASSIGN",		
	"TOK_OP_STAR_ASSIGN",		
	"TOK_OP_DIV_ASSIGN",		
	"TOK_OP_MOD_ASSIGN",		
	"TOK_OP_INCREASE",			
	"TOK_OP_DECREASE",			
	"TOK_OP_ASSIGN",			
	"TOK_LOG_GREATER",			
	"TOK_LOG_LESSER",			
	"TOK_LOG_GREATER_OR_EQUAL",	
	"TOK_LOG_LESSER_OR_EQUAL",	
	"TOK_LOG_EQUAL",			
	"TOK_LOG_NOT_EQUAL",		
	"TOK_LOG_NOT",				
	"TOK_LOG_AND",				
	"TOK_LOG_OR",				
	"TOK_BIT_AMPERSAND",
	"TOK_BIT_NOT",
	"TOK_BIT_OR",
	"TOK_BIT_EOR",
	"TOK_BIT_LEFT_SHIFT",
	"TOK_BIT_RIGHT_SHIFT",
	"TOK_BIT_AMPERSAND_ASSIGN",
	"TOK_BIT_NOT_ASSIGN",
	"TOK_BIT_OR_ASSIGN",
	"TOK_BIT_EOR_ASSIGN",
	"TOK_BIT_LEFT_SHIFT_ASSIGN",
	"TOK_BIT_RIGHT_SHIFT_ASSIGN",
	"TOK_CTRL_PAREN_OPEN",
	"TOK_CTRL_PAREN_CLOSED",	
	"TOK_CTRL_BRACKET_OPEN",	
	"TOK_CTRL_BRACKET_CLOSED",	
	"TOK_CTRL_BRACE_OPEN",		
	"TOK_CTRL_BRACE_CLOSED",	
	"TOK_CTRL_DOT",				
	"TOK_CTRL_ARROW",			
	"TOK_CTRL_SEMIKOLON",		
	"TOK_CTRL_KOLON",			
	"TOK_CTRL_COMMA",			
	"TOK_CTRL_ELLIPSIS",
	"TOK_CTRL_QUERY",			
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

struct lex_token
{
	enum				lex_token_type type;
	struct				err_location err_loc;
	union const_value	value;
	int					identifier_id;
};

struct lex_file_buffer
{
	uint32_t			line;
	uint32_t			col;

	char*				filename;
	
	char*				start;
	char*				curr;
	char*				end;

	struct lex_token	last;
};

void
lex_load_file_buffer(struct lex_file_buffer* file_buffer, const char* filename);

void
lex_cleanup_file_buffer(struct lex_file_buffer* file_buffer);

struct lex_token
lex_next_token(struct lex_file_buffer* fb, struct sym_table* table);

#endif
