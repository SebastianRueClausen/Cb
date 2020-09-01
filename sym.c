#include "sym.h"
#include "def.h"

#include <stdio.h>

// murmur hash
int64_t sym_hash(const char* key, int len)
{
	int i;
	int64_t h = 525201411107845655;
	for (i = 0; i < len; ++i) {
		h ^= key[i];
		h *= 0x5bd1e9955bd1e995;
		h ^= h >> 47;
	}

	return h;
}

void
sym_create_table(struct sym_table* table, size_t count)
{
	table->entry_allocated	= count;
	table->entries			= c_malloc(sizeof(struct sym_entry) * count);
	table->entry_count		= 0;
}

int
sym_add_entry(struct sym_table* table, struct sym_entry entry)
{
	if (table->entry_allocated == table->entry_count) {
		// We double the size for now
		table->entry_allocated *= 2;
		table->entries = c_realloc(
			table->entries, sizeof(struct sym_entry) * table->entry_allocated
		);
	}

	table->entries[table->entry_count] = entry;
	return table->entry_count++;	
}

void
sym_cleanup_table(struct sym_table* table)
{
	table->entry_allocated	= 0;	
	table->entry_count		= 0;
	c_free(table->entries);
}

int
sym_find_entry(const struct sym_table* table, int64_t hash)
{
	int i;

	for (i = 0; i < table->entry_count; ++i) {
		if (table->entries[i].hash == hash) {
			return i;
		}
	}

	return -1;
}




