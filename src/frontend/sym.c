#include "frontend.h"

#include <stdio.h>
#include <assert.h>

/* 64-bit murmur hash */
sym_hash_t
sym_hash(const char *key, uint32_t len)
{
	uint32_t i;
	int64_t h = 525201411107845655;
	for (i = 0; i < len; ++i) {
		h ^= key[i];
		h *= 0x5bd1e9955bd1e995;
		h ^= h >> 47;
	}

	return h;
}

/* allocates a new symbol table */
void
sym_create_table(sym_table_t *table, uint32_t count)
{
	table->entries = vec_sym_entry_t_create(count);
	table->hashes = vec_sym_hash_t_create(count);


	/* create global scope */
	const sym_scope_t global_scope =
	{
		.start	= 0,
		.end	= 0
	};

	table->scopes = vec_sym_scope_t_create(5);
	vec_sym_scope_t_push(&table->scopes, global_scope);
}

/* push a new scope */
void
sym_push_scope(sym_table_t *table)
{
	const sym_scope_t scope =
	{
		.start = vec_sym_scope_t_top(&table->scopes).end,
		.end = scope.start,
	};
	
	vec_sym_scope_t_push(&table->scopes, scope);
}

/* pop the top scope, and remove removed */
void
sym_pop_scope(sym_table_t *table)
{
	uint32_t new_top;

	/* scope 1, is global */
	if (table->scopes.size == 1) {
		assert(false);
	}

	new_top = vec_sym_scope_t_top(&table->scopes).start;

	/* resize the entries */
	vec_sym_entry_t_resize(&table->entries, new_top);
	vec_sym_hash_t_resize(&table->hashes, new_top);

	/* pop the current scope */
	vec_sym_scope_t_pop(&table->scopes);
}

/* checks that there is a enough space allocated for a new entry, if not
 * it doubles the size of the table */
uint32_t
sym_add_entry(sym_table_t *table, sym_entry_t entry, sym_hash_t hash)
{
	vec_sym_entry_t_push(&table->entries, entry);
	vec_sym_hash_t_push(&table->hashes, hash);

	/* increase the reach of the scope */
	++vec_sym_scope_t_top_ptr(&table->scopes)->end;		
	
	return table->entries.size;
}

/* free the symbol table */
void
sym_destroy_table(sym_table_t *table)
{
	vec_sym_entry_t_destroy(&table->entries);
	vec_sym_hash_t_destroy(&table->hashes);

	vec_sym_scope_t_destroy(&table->scopes);
}


/* looks through every entry in table and return the index if found. -1 otherwise */
int32_t
sym_find_id(const sym_table_t *table, sym_hash_t hash)
{
	int32_t i;

	/* @performance: should we loop through the table in reverse?,
	 * since you often reference newly created variables */
	/* @todo: perhaps there should be some kind of foreach? */
	for (i = 0; (uint32_t)i < table->hashes.size; ++i) {
		if (table->hashes.data[i] == hash) {
			return i;
		}
	}

	return SYM_NOT_FOUND;
}

/* get a symbol entry ptr from an id */
sym_entry_t*
sym_get_entry(const sym_table_t *table, uint32_t id)
{
	if (id > table->entries.size) {
		fatal_error("[Internal]: failed lookup for sym_entry");
	}

	return &table->entries.data[id];	
}

void
sym_print(const sym_entry_t *entry)
{
	uint32_t i;

	printf("entry: \n");
	// printf("\ttype: %s\n", sym_prim_str[entry->prim]);
	printf("\tindirection: %u\n", entry->type.indirection);

	for (i = 0; i < 9; ++i) {
		// TODO make this work again
		// printf("\t%s: %s\n", [i], (entry->spec & (1<< i) ? "X" : "0"));
	}

}
