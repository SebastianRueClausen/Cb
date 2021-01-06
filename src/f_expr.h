#ifndef _F_EXPR_
#define _F_EXPR_

#include "f_ast.h"
#include "f_lexer.h"
#include "symbol.h"
#include "err.h"

ast_node_t* f_parse_expression(parser_t* parser, int32_t prev_prec, token_type_t terminator);
void check_argument_list(parser_t *parser, ast_node_t *func_call);

#endif
