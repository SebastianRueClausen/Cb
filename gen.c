#include "gen.h"

int
gen_ast(struct ast_node* node)
{
	int left, right;

	if (node->left)		left  = gen_ast(node->left);
	if (node->right)	right = gen_ast(node->right);

	switch (node->type) {
		case AST_ADD:		return;
		
	}
}
