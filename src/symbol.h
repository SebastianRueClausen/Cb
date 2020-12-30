#ifndef _SYMBOL_
#define _SYMBOL_

#include "type.h"

#include <limits.h>
#include <stdint.h>

/* returned if no symbol is found */
#define SYM_ID_GLOBAL_NULL	INT32_MIN
#define SYM_ID_LOCAL_NULL	INT32_MAX
#define SYM_ID_NULL			0
#define SYM_NULL_HASH		0

typedef int64_t sym_hash_t;
typedef int32_t	sym_id_t;

typedef enum sym_local_kind
{
	SYM_LOCAL_KIND_VARIABLE,
	SYM_LOCAL_KIND_PARAMETER

} sym_local_kind_t;

typedef enum sym_global_kind
{
	SYM_GLOBAL_KIND_VARIABLE,
	SYM_GLOBAL_KIND_FUNCTION

} sym_global_kind_t;

typedef struct sym_param
{
	type_info_t				type;
	sym_hash_t				hash;

	/* ugh, we have to store an error loc, since it might be an error later */
	err_location_t			err_loc;

} sym_param_t;

#define VEC_TYPE sym_param_t
#include "templates/vec.h"
#undef VEC_TYPE

/* would prob be better to have a list of global, and local
 * entry list, since globals take up more data */
typedef struct sym_local
{
	type_info_t				type;
	sym_local_kind_t		kind;

} sym_local_t;

typedef struct sym_global
{
	type_info_t				type;
	sym_global_kind_t		kind;

	bool					defined;

	union
	{
		struct
		{
			vec_sym_param_t		params;

		} function;

		value_t					val;
	};

} sym_global_t;

/* points to a section of the symbol table */
typedef struct sym_scope
{
	uint32_t				start;
	uint32_t				end;

} sym_scope_t;

/* generate vectors */
#define VEC_TYPE sym_local_t
#include "templates/vec.h"
#undef VEC_TYPE

#define VEC_TYPE sym_global_t
#include "templates/vec.h"
#undef VEC_TYPE

#define VEC_TYPE sym_scope_t
#include "templates/vec.h"
#undef VEC_TYPE

#define VEC_TYPE sym_hash_t
#include "templates/vec.h"
#undef VEC_TYPE

typedef struct sym_table
{
	vec_sym_global_t		globals;
	vec_sym_local_t			locals;

	vec_sym_hash_t			global_hashes;
	vec_sym_hash_t			local_hashes;	

	vec_sym_scope_t			scopes;

} sym_table_t;

sym_hash_t			sym_hash(const char *key, uint32_t len);

void				sym_create_table(sym_table_t *table, uint32_t count);
void				sym_destroy_table(sym_table_t *table);

sym_id_t			sym_find_global(const sym_table_t *table, sym_hash_t hash);
sym_global_t*		sym_get_global(sym_table_t *table, sym_id_t id);
sym_id_t			sym_declare_global(sym_table_t *table, sym_global_t global, sym_hash_t hash, err_location_t *err_loc);
sym_id_t			sym_define_global(sym_table_t *table, sym_global_t global, sym_hash_t hash, err_location_t *err_loc);


sym_id_t			sym_find_local(const sym_table_t *table, sym_hash_t hash);
sym_local_t*		sym_get_local(sym_table_t *table, sym_id_t id);
sym_id_t			sym_define_local(sym_table_t *table, sym_local_t local, sym_hash_t hash, err_location_t *err_loc);

void*				sym_get_entry(sym_table_t *table, sym_id_t id);
sym_id_t			sym_find_id(const sym_table_t *table, sym_hash_t hash);

void				sym_push_scope(sym_table_t *table);
void				sym_pop_scope(sym_table_t *table);
void				sym_add_params_to_scope(sym_table_t *table, const vec_sym_param_t *params);

void				sym_check_for_anon_params(const vec_sym_param_t *params);
void				sym_check_for_duplicate_params(const vec_sym_param_t *params);





#endif
