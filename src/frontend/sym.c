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
	table->locals = vec_sym_local_t_create(count);
	table->globals = vec_sym_global_t_create(count);

	table->global_hashes = vec_sym_hash_t_create(count);
	table->local_hashes = vec_sym_hash_t_create(count);

	/* create global scope */
	table->scopes = vec_sym_scope_t_create(5);
}

/* push a new scope */
void
sym_push_scope(sym_table_t *table)
{
	sym_scope_t scope;

	scope.size = 0;

	printf("push scope count %lu\n", table->locals.size);
	
	vec_sym_scope_t_push(&table->scopes, scope);
}

/* pop the top scope, and remove removed */
void
sym_pop_scope(sym_table_t *table)
{
	uint32_t new_top =
		table->locals.size - vec_sym_scope_t_top(&table->scopes).size;

	printf("pop scope count before %lu\n", table->local_hashes.size);

	/* resize the entries */
	vec_sym_local_t_resize(&table->locals, new_top);
	vec_sym_hash_t_resize(&table->local_hashes, new_top);

	printf("pop scope count after %lu\n", table->local_hashes.size);

	/* pop the current scope */
	vec_sym_scope_t_pop(&table->scopes);
}

static void
check_for_redeclaration(sym_global_t *new, sym_global_t *old,
					   err_location_t *err_loc)
{
	uint32_t i;	

	if (!type_compare(new->type, old->type) ||
		new->kind != old->kind) {
		syntax_error(*err_loc, "conflicting types for declaration of global symbol1");
	}

	/* check parameters match */
	if (new->kind == SYM_GLOBAL_KIND_FUNCTION) {

		/* if number of parameters doesnt match */
		if (new->function.params.size != old->function.params.size) {
			syntax_error(*err_loc, "conflicting types for declaration of global symbol2");
		}

		/* check the parameters match */
		for (i = 0; i < new->function.params.size; ++i) {
			if (!type_compare(new->function.params.data[i].type, old->function.params.data[i].type)) {
				syntax_error(*err_loc, "conflicting types for declaration of global symbol3");
			}
		}
	}
}

sym_id_t
sym_find_global(const sym_table_t *table, sym_hash_t hash)
{
	sym_hash_t *current, *start, *end;

	start = table->global_hashes.data;
	end = table->global_hashes.data + table->global_hashes.size + 1;

	/* @todo: perhaps there should be some kind of foreach? */
	for (current = start; current != end; ++current) {
		if (*current == hash) {
			return (current - start + 1) * -1;
		}
	}

	return SYM_ID_GLOBAL_NULL;
}

static sym_id_t
add_global(sym_table_t *table, sym_global_t global, sym_hash_t hash)
{
	vec_sym_global_t_push(&table->globals, global);
	vec_sym_hash_t_push(&table->global_hashes, hash);

	assert(table->globals.size == table->global_hashes.size);

	return -(table->globals.size);		
}

sym_global_t*
sym_get_global(sym_table_t *table, sym_id_t id)
{
	assert(id < 0);

	return &table->globals.data[(id * -1) - 1];
}

sym_id_t
sym_declare_global(sym_table_t *table, sym_global_t global, sym_hash_t hash, err_location_t *err_loc)
{
	sym_id_t id;
	sym_global_t *sym;	

	assert(table->scopes.size == 0);

	id = sym_find_global(table, hash);


	if (id == SYM_ID_GLOBAL_NULL) {
		global.defined = false;
		return add_global(table, global, hash);	
	}

	sym = sym_get_global(table, id);

	/* if it is already defined we just check that the definitions match */
	check_for_redeclaration(&global, sym, err_loc);
	
	return id;		
}

sym_id_t
sym_define_global(sym_table_t *table, sym_global_t global, sym_hash_t hash, err_location_t *err_loc)
{
	sym_id_t id;
	sym_global_t *sym;	

	assert(table->scopes.size == 0);

	id = sym_find_global(table, hash);

	if (id == SYM_ID_GLOBAL_NULL) {
		global.defined = true;
		return add_global(table, global, hash);
	}

	assert(id < 0);
	sym = sym_get_global(table, id);

	if (sym->defined) {
		syntax_error(*err_loc, "redefinition of global");
	}

	check_for_redeclaration(&global, sym, err_loc);
	
	sym->defined = true;

	return id;
}

sym_id_t
sym_find_local(const sym_table_t *table, sym_hash_t hash)
{
	sym_hash_t *current, *start, *end;

	start = table->local_hashes.data - 1;
	end = table->local_hashes.data + table->local_hashes.size - 1;

	/* loop through in reverse */
	for (current = end; current != start; --current) {
		if (*current == hash) {
			return (current - start) + 1;
		}
	}

	return SYM_ID_LOCAL_NULL;
}

static sym_id_t
add_local(sym_table_t *table, sym_local_t local, sym_hash_t hash)
{
	vec_sym_local_t_push(&table->locals, local);
	vec_sym_hash_t_push(&table->local_hashes, hash);

	assert(table->locals.size == table->local_hashes.size);

	/* update current scope */
	vec_sym_scope_t_top_ptr(&table->scopes)->size++;

	/* local id are positive */
	return table->locals.size;
}

sym_local_t*
sym_get_local(sym_table_t *table, sym_id_t id)
{
	assert(id >= 0);

	return &table->locals.data[id - 1];
}

static sym_id_t
search_current_scope(sym_table_t *table, sym_hash_t hash)
{
	sym_hash_t *current, *start, *end;

	end = table->local_hashes.data + table->local_hashes.size - 1;
	start = end - vec_sym_scope_t_top(&table->scopes).size;

	/* loop through in reverse */
	for (current = end; current != start; --current) {
		if (*current == hash) {
			return (current - start) + 1;
		}
	}

	return SYM_ID_LOCAL_NULL;
}


/* @todo: you should be able to define a new variable by
 * same name in a deeper scope */
sym_id_t
sym_define_local(sym_table_t *table, sym_local_t local, sym_hash_t hash, err_location_t *err_loc)
{
	sym_id_t id = search_current_scope(table, hash);

	if (id != SYM_ID_LOCAL_NULL) {
		syntax_error(*err_loc, "redefinition of variable");	
	}

	return add_local(table, local, hash);
}

void
sym_check_for_anon_params(const vec_sym_param_t *params)
{
	uint32_t i;

	for (i = 0; i < params->size; ++i) {
		if (params->data[i].hash == SYM_NULL_HASH) {
			syntax_error(params->data[i].err_loc, "anon parameter");
		}
	}
}

void
sym_check_for_duplicate_params(const vec_sym_param_t *params)
{
	uint32_t i, j;
	sym_hash_t hash;

	for (i = 0; i < params->size; ++i) {
		hash = params->data[i].hash;
		for (j = 0; j < i; ++j) {
			/* we allow anon params */
			if (hash == params->data[j].hash &&
					hash != SYM_NULL_HASH) {
				syntax_error(params->data[i].err_loc, "redefinition of parameter");
			}
		}
	}
}

/* called when we parse a function declaration,
 * but it's a function definition, so we have to add
 * the params to scope */

void
sym_add_params_to_scope(sym_table_t *table, const vec_sym_param_t *params)
{
	uint32_t i;
	sym_local_t local;

	local.kind = SYM_LOCAL_KIND_PARAMETER;

	for (i = 0; i < params->size; ++i) {
		local.type = params->data[i].type;
		add_local(table, local, params->data[i].hash);
	}
}

/* return global or local entry */
void*
sym_get_entry(sym_table_t *table, sym_id_t id)
{
	if (id > 0) {
		return sym_get_local(table, id);
	} else if (id < 0) {
		return sym_get_global(table, id);
	} else {
		assert(false);
	}
}

sym_id_t
sym_find_id(const sym_table_t *table, sym_hash_t hash)
{
	sym_id_t id;

	id = sym_find_local(table, hash);

	if (id != SYM_ID_LOCAL_NULL) {
		return id;	
	}

	id = sym_find_global(table, hash);

	if (id != SYM_ID_GLOBAL_NULL) {
		return id;	
	}

	return SYM_ID_NULL;
}


/* free the symbol table */
void
sym_destroy_table(sym_table_t *table)
{
	vec_sym_local_t_destroy(&table->locals);
	vec_sym_global_t_destroy(&table->globals);

	vec_sym_hash_t_destroy(&table->local_hashes);
	vec_sym_hash_t_destroy(&table->global_hashes);

	vec_sym_scope_t_destroy(&table->scopes);
}

