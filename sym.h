#ifndef SYM_H
#define SYM_H

#include "def.h"
#include "type.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* returned if no symbol is found */
#define SYM_NOT_FOUND -1

enum sym_kind
{
	SYM_KIND_FUNCTION,
	SYM_KIND_VARIABLE
};

enum sym_compat
{
	SYM_COMPAT_INCOMPAT,
	SYM_COMPAT_COMPAT,
	SYM_COMPAT_WIDEN_LEFT,
	SYM_COMPAT_WIDEN_RIGHT
};

struct sym_entry
{
	struct type_info		type;
	enum sym_kind			kind;
	int64_t					hash;
};

struct sym_table
{
	struct sym_entry		*entries;
	uint32_t				entry_count;
	uint32_t				entry_allocated;
};

int64_t
sym_hash(const char *key, uint32_t len);

void
sym_create_table(struct sym_table *table, uint32_t count);

uint32_t
sym_add_entry(struct sym_table *table, struct sym_entry entry);

void
sym_destroy_table(struct sym_table *table);

int32_t
sym_find_id(const struct sym_table *table, int64_t hash);

struct sym_entry*
sym_get_entry(const struct sym_table *table, uint32_t id);

void
sym_print(const struct sym_entry *entry);

#endif
