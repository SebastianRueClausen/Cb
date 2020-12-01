#ifndef SSA_H
#define SSA_H

#include "lex.h"
#include "ast.h"
#include "sym.h"
#include "type.h"

/*
 * three address code is the next step from ast, which we use to optimize the code
 * and turn it into assembly.
 * It's a list of sequential instructions, which contains one operation
 * and two sources
 */

/* type of operation */
enum ssa_op
{
	ssa_OP_ADD,
	ssa_OP_SUB,
	ssa_OP_MUL,
	ssa_OP_DIV,
	ssa_OP_ASSIGN
};

struct ssa_instruction;

/* an address may refer a prev instruction, a symbol or a const val */
union ssa_address
{
	const struct ssa_instruction	*ins;
	int								sym_id;	
	union type_literal_value		val;
};

enum ssa_address_type
{
	ssa_TYPE_INS,
	ssa_TYPE_SYM,
	ssa_TYPE_VAL
};

/* a single instruction */
struct ssa_instruction
{
	enum ssa_op						op;

	union ssa_address				addr[2];
	enum ssa_address_type			addr_type[2];
};

/* table of instruction */
struct ssa_table
{
	int								allocated;
	int								count;
	struct ssa_instruction			*instructions;
};

void
ssa_test(const struct ast_node *ast);

#endif
