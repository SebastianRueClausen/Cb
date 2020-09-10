#ifndef SYM_H
#define SYM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum sym_spec
{
	SYM_CONST				= 1 << 0,
	SYM_EXTERN				= 1 << 1,
	SYM_LONG				= 1 << 2,
	SYM_REGISTER			= 1 << 3,
	SYM_SIGNED				= 1 << 4,
	SYM_STATIC				= 1 << 5,
	SYM_UNSIGNED			= 1 << 6,
	SYM_VOLATILE			= 1 << 7,
	SYM_SHORT				= 1 << 8
};

enum sym_type
{
	SYM_NONE, SYM_INT, SYM_FLOAT, SYM_CHAR, SYM_DOUBLE
};

struct sym_entry
{
	enum sym_spec			specifiers;
	enum sym_type			type;
	int64_t					hash;
};


struct sym_table
{
	struct sym_entry		*entries;
	int						entry_count;
	int						entry_allocated;
};

int64_t
sym_hash(const char *key, int len);

void
sym_create_table(struct sym_table *table, int count);

int
sym_add_entry(struct sym_table *table, struct sym_entry entry);

void
sym_cleanup_table(struct sym_table *table);

int
sym_find_entry(const struct sym_table *table, int64_t hash);

void
sym_print(const struct sym_entry *entry);

#endif
