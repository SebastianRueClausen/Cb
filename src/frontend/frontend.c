#include "frontend.h"

void
frontend_create(frontend_t *f, const char* filename)
{
	f->lex = lex_create_instance(filename);

	sym_create_table(&f->sym_table, 20);

	f->pool = mem_pool_create(1024);

	/* we need to lex the first node first */
	lex_next_token(f);
}

void
frontend_destroy(frontend_t *f)
{
	lex_destroy_instance(&f->lex);
	sym_destroy_table(&f->sym_table);
	mem_pool_destroy(&f->pool);
}
