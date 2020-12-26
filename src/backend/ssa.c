#include "ssa.h"

#include <assert.h>
#include <stdio.h>

/* create a new table if*/
struct ssa_table
ssa_create_table(int ins_count)
{
	struct ssa_table table;

	table.allocated = ins_count;
	table.instructions = c_malloc(sizeof(struct ssa_instruction) * ins_count);

	assert(table.instructions);

	return table;
}

/* create a new entry in the table and the list, and make sure
 * there is enough space allocated */
static struct ssa_instruction*
ssa_add_instriction(struct ssa_table *table, struct ssa_instruction ins)
{
	/* double the size of the allocation if out of space */
	if (table->allocated == table->count) {
		table->allocated *= 2;
		table->instructions = c_realloc(table->instructions, sizeof(struct ssa_instruction) * table->allocated);
		assert(table->instructions);
	}

	table->instructions[table->count] = ins;
	++table->count;

	return &table->instructions[table->count - 1];
}

static void
ssa_ast_to_ssa(struct ssa_table *table, const struct ast_node *ast)
{
	struct ssa_instruction ins;

	/*
	switch (ast->type) {
		case AST_ADD:
			ins.op = ssa_OP_ADD;
		
			if (ast->left->type == AST_IDENTIFIER) {
				ins.addr_type[0] = ssa_TYPE_SYM;
				ins.addr[0].sym_id = ast->left->sym_id;
			}
			break;

		default:
			assert(false);
	}
	*/

	ins.op = ast->type;

	ssa_add_instriction(table, ins);
}

void
ssa_create_from_ast(struct ssa_table *table, const struct ast_node *ast)
{
	assert(ast);

	if (ast->left) {
		ssa_create_from_ast(table, ast->left);	
	}

	if (ast->center) {
		ssa_create_from_ast(table, ast->center);	
	}

	if (ast->right) {
		ssa_create_from_ast(table, ast->right);	
	}

	ssa_ast_to_ssa(table, ast);
}

static void
print_ssa(struct ssa_table *table)
{
	int i;

	for (i = 0; i < table->count; ++i) {
		printf("%s\n", ast_to_str((enum ast_type)table->instructions[i].addr_type));
	}
}

void
ssa_test(const struct ast_node *ast)
{
	struct ssa_table table;

	table = ssa_create_table(10);
	
	ssa_create_from_ast(&table, ast);
	
	print_ssa(&table);
}


