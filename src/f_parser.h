#ifndef _F_PARSER_
#define _F_PARSER_

#include "type.h"
#include "symbol.h"
#include "mem.h"

#include "f_lexer.h"
#include "f_ast.h"

typedef struct parser
{
    mem_pool_t   pool;
    lexer_t *    lexer;
    sym_table_t *sym_table;

} parser_t;

void f_create_parser(parser_t *parser, lexer_t *lexer, sym_table_t *table);
void f_destroy_parser(parser_t *parser);

/* @debug: should not be public */
ast_node_t *parse_statement(parser_t *parser);

ast_node_t *f_generate_ast(parser_t *parser);

#endif
