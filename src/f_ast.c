#include "f_ast.h"

#include <stdio.h>

#define DEBUG 1 

#ifdef DEBUG

static const char* ast_str[] =
{	
	"AST_UNDEF",
	"AST_ADD",
	"AST_MIN",
	"AST_MUL",
	"AST_DIV",
	"AST_MOD",

	"AST_EQUAL",
	"AST_NOT_EQUAL",
	"AST_LESSER",
	"AST_GREATER",
	"AST_LESSER_EQUAL",
	"AST_GREATER_EQUAL",

	"AST_UNARY_PLUS",
	"AST_UNARY_MINUS",
	"AST_UNARY_NOT",

	"AST_BIT_NOT",

	"AST_PRE_INCREMENT",
	"AST_PRE_DECREMENT",

	"AST_POST_INCREMENT",
	"AST_POST_DECREMENT",

	"AST_DEREF",
	"AST_ADDRESS",

	"AST_PROMOTE",

	"AST_ASSIGN",

	"AST_LITERAL",
	
	"AST_IF",
	"AST_WHILE",

	"AST_IDENTIFIER",

	"AST_NOP"
};

const char*
f_ast_type_to_str(ast_type_t type)
{
	return ast_str[type];
}

#else

const char*
f_ast_type_to_str(ast_type_t type)
{
	return "Error: Trying to print ast_type in RELEASE build";
}

#endif


/* @debug: make this easier to read in the console */
void
f_print_ast(ast_node_t *node, uint32_t level)
{
	uint32_t i;

	++level;
	printf("%s\n", f_ast_type_to_str(node->type));

	if (node->left) {
		for (i = 0; i < level; ++i) {
			printf("\t");
		}
		
		f_print_ast(node->left, level);			
	}
	
	if (node->center) {
		for (i = 0; i < level; ++i) {
			printf("\t");
		}

		f_print_ast(node->center, level);			
	}

	if (node->right) {
		for (i = 0; i < level; ++i) {
			printf("\t");
		}

		f_print_ast(node->right, level);
	}
}

void
f_print_ast_postorder(ast_node_t *node)
{
	if (!node) {
		return;
	}

	if (node->left) {
		f_print_ast_postorder(node->left);
	}

	if (node->center) {
		f_print_ast_postorder(node->center);
	}

	if (node->right) {
		f_print_ast_postorder(node->right);
	}

	printf("%s\n", f_ast_type_to_str(node->type));

}


