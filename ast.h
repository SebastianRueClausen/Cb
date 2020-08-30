#ifndef AST_H
#define AST_H

enum ast_type
{
	AST_UNDEF, AST_ADD, AST_MIN, AST_MUL, AST_DIV, AST_INT, _AST_COUNT
};

static const char* ast_type_to_str[] = {	
	"AST_UNDEF", "AST_ADD", "AST_MIN", "AST_MUL", "AST_DIV", "AST_INT"
};

struct ast_node
{
	enum ast_type type;

	union
	{
		long long	long_value;
		double		double_value;
		int			int_value;
		float		float_value;
		char		char_value;
		short		short_value;
	};
	
	struct ast_node* left;
	struct ast_node* right;
};

#endif
