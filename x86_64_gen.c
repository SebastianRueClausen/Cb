#include "x86_64_gen.h"
#include "def.h"

#include <stdbool.h>
#include <assert.h>

static const char* reg_str[] = { "%r8", "%r9", "%r10", "%r11" };

static void reg_free_all(struct x86_64_gen* gen)
{
	gen->reg_alloc[0] = false;
	gen->reg_alloc[1] = false;
	gen->reg_alloc[2] = false;
	gen->reg_alloc[3] = false;
}

static int reg_alloc(struct x86_64_gen* gen)
{
	int i;

	for (i = 0; i < 4; ++i) {
		if (!gen->reg_alloc[i])	{
			gen->reg_alloc[i] = true;
			return i;
		}
	}

	assert(false);
}

static void reg_free(struct x86_64_gen* gen, int reg)
{
	assert(gen->reg_alloc[reg] == true);
	gen->reg_alloc[reg] = false;
}

int x86_64_load(struct x86_64_gen* gen, union const_value value)
{
	int reg = reg_alloc(gen);	

}

int x86_64_add(struct x86_64_gen* gen, int left_reg, int right_reg);
int x86_64_sub(struct x86_64_gen* gen, int left_reg, int right_reg);
int x86_64_mul(struct x86_64_gen* gen, int left_reg, int right_reg);
int x86_64_div(struct x86_64_gen* gen, int left_reg, int right_reg);
