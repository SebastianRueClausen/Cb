#ifndef SSA_H
#define SSA_H

#include "../frontend/frontend.h"

/*
 * three address code is the next step from ast, which we use to optimize the code
 * and turn it into assembly.
 * It's a list of sequential instructions, which contains one operation
 * and two sources
 */

/* type of operation */
typedef enum ssa_op
{
	SSA_OP_ADD,
	SSA_OP_SUB,
	SSA_OP_MUL,
	SSA_OP_DIV,
	SSA_OP_ASSIGN
}
ssa_op_t;

typedef struct ssa_instruction ssa_instruction_t;

/* an address may refer a prev instruction, a symbol or a const val */
typedef union ssa_address
{
	const ssa_instruction_t			*ins;
	int								sym_id;	
	type_literal_value_t			val;
}
ssa_address_t;

typedef enum ssa_address_type
{
	SSA_TYPE_INS,
	SSA_TYPE_SYM,
	SSA_TYPE_VAL
}
ssa_address_type_t;

/* a single instruction */
typedef struct ssa_instruction
{
	ssa_op_t						op;
	ssa_address_t					addr[2];
	ssa_address_type_t				addr_type[2];
}
ssa_instruction_t;

/* table of instruction */
typedef struct ssa_table
{
	int								allocated;
	int								count;
	ssa_instruction_t				*instructions;
}
ssa_table_t;

void
ssa_test(const ast_node_t *ast);

#endif
