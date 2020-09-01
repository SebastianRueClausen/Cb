#ifndef X86_64_GEN_H
#define X86_64_GEN_H

#include <stdbool.h>
#include <stdio.h>

struct x86_64_gen
{
	FILE* out_file;
	bool reg_alloc[4];
};

int x86_64_load(struct x86_64_gen* gen, int val);

int x86_64_add(struct x86_64_gen* gen, int left_reg, int right_reg);
int x86_64_sub(struct x86_64_gen* gen, int left_reg, int right_reg);
int x86_64_mul(struct x86_64_gen* gen, int left_reg, int right_reg);
int x86_64_div(struct x86_64_gen* gen, int left_reg, int right_reg);

#endif
