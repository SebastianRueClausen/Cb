#ifndef INS_H
#define INS_H

#include "def.h"

#include <stdbool.h>

/*
 *
 * When we have created a abtract syntax tree from the token stream,
 * we generate a list generic instructions, which we can use to optimize and
 * generate platform specific assembly/machine code
 *
 */

enum ins_type
{
	INS_NOP,
	INS_INIT,
	INS_COPY,
	INS_READ,
	INS_WRITE,

	INS_ADD,
	INS_SUB,
	INS_DIV,
	INS_MUL,

	INS_NEGATIVE,
	INS_EQUAL,
	INS_IF_NOT,
	INS_CALL,
	INS_RETURN
};

struct ins_arg
{
	enum 
	{
		INS_ARG_VAL,
		INS_ARG_SYM_ID,
		INS_ARG_TEMP_ID
	}									type;

	union
	{
		union const_value				value;
		int								sym_id;			// symbol table entry
		int								tmp_id;			// temp variable id
	};
};

struct ins_statement
{
	struct ins_arg						arg_1;
	struct ins_arg						arg_2;
	struct ins_arg						result;

	enum ins_type						type;
};



#endif
