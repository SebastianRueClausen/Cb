#ifndef SYM_H
#define SYM_H

#include <stdint.h>
#include <stddef.h>

struct sym_entry
{
	int64_t	hash;
};

struct sym_table
{
	struct sym_entry*		entries;
	size_t					entry_count;
	size_t					entry_allocated;
};

int64_t
sym_hash(const char* key, int len);

void
sym_create_table(struct sym_table* table, size_t count);

int
sym_add_entry(struct sym_table* table, struct sym_entry entry);

void
sym_cleanup_table(struct sym_table* table);

int
sym_find_entry(const struct sym_table* table, int64_t hash);

#endif
