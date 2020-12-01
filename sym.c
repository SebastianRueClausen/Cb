#include "sym.h"

#include <stdio.h>

/* 64-bit murmur hash */
int64_t
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
sym_create_table(struct sym_table *table, uint32_t count)
{
	table->entry_allocated = count;
	table->entries = c_malloc(sizeof(struct sym_entry) * count);
	table->entry_count = 0;
}

/* checks that there is a enough space allocated for a new entry, if not
 * it doubles the size of the table */
uint32_t
sym_add_entry(struct sym_table *table, struct sym_entry entry)
{
	if (table->entry_allocated == table->entry_count) {
		/* We double the size for now */
		table->entry_allocated *= 2;
		table->entries = c_realloc(table->entries, sizeof(struct sym_entry) *
													   table->entry_allocated);
	}

	table->entries[table->entry_count] = entry;
	
	return table->entry_count++;
}

/* free the symbol table */
void
sym_destroy_table(struct sym_table *table)
{
	table->entry_allocated = 0;
	table->entry_count = 0;
	c_free(table->entries);
}


/* looks through every entry in table and return the index if found. -1 otherwise */
int32_t
sym_find_id(const struct sym_table *table, int64_t hash)
{
	int32_t i;

	// @performance: should we loop through the table in reverse?,
	// since you often reference newly created variables 
	for (i = 0; (uint32_t)i < table->entry_count; ++i) {
		if (table->entries[i].hash == hash) {
			return i;
		}
	}

	return SYM_NOT_FOUND;
}

/* get a symbol entry from an id */
struct sym_entry*
sym_get_entry(const struct sym_table *table, uint32_t id)
{
	// @note should this return NULL?
	if (id > table->entry_count) {
		fatal_error("[Internal]: failed lookup for sym_entry");
	}
	
	return &table->entries[id];	
}

void
sym_print(const struct sym_entry *entry)
{
	uint32_t i;

	printf("entry: \n");
	printf("\thash: %li\n", entry->hash);
	// printf("\ttype: %s\n", sym_prim_str[entry->prim]);
	printf("\tindirection: %u\n", entry->type.indirection);

	for (i = 0; i < 9; ++i) {
		// printf("\t%s: %s\n", sym_spec_str[i], (entry->spec & (1<< i) ? "X" : "0"));
	}

}
