#include "f_expr.h"
#include "f_parser.h"
#include "type.h"

#include <stdio.h>

/* builds an ast tree from an expression */

inline static bool
is_rvalue(token_type_t tok)
{
    switch (tok)
    {
    case TOK_LITERAL:
        return true;
    default:
        return false;
    }
}

inline static bool
is_lvalue(token_type_t tok)
{
    switch (tok)
    {
    case TOK_IDENTIFIER:
        return true;
    default:
        return false;
    }
}

/* operation tokens used in expressions */
/* @perforamce: make sections lign up with token type for farster,
 * conversion */
static ast_type_t
op_tok_to_ast(token_type_t tok)
{
    switch (tok)
    {
    case TOK_PLUS:
        return AST_ADD;
    case TOK_MINUS:
        return AST_MIN;
    case TOK_STAR:
        return AST_MUL;
    case TOK_DIV:
        return AST_DIV;
    case TOK_MOD:
        return AST_MOD;
    case TOK_EQUAL:
        return AST_EQUAL;
    case TOK_NOT_EQUAL:
        return AST_NOT_EQUAL;
    case TOK_GREATER:
        return AST_GREATER;
    case TOK_GREATER_OR_EQUAL:
        return AST_GREATER_EQUAL;
    case TOK_LESSER:
        return AST_LESSER;
    case TOK_LESSER_OR_EQUAL:
        return AST_LESSER_EQUAL;
    case TOK_ASSIGN:
        return AST_ASSIGN;
    case TOK_COMMA:
        return AST_LIST;
    default:
        return AST_NULL;
    }
}

/* since the type info can be represented in different ways in ast nodes,
 * this function gets the right one dependent on ast_type */
static type_info_t
ast_get_type_info(parser_t *parser, ast_node_t *node)
{
    switch (node->type)
    {
    case AST_LITERAL:
        return type_from_literal(node->literal);

    case AST_IDENTIFIER:
        return sym_get_type_info(parser->sym_table, node->sym_id);

    /* we just assume its some other node as part of and expression,
		 * or an unary node, in which case we also save
		 * the type of what its pointing at in expr_type*/
    default:
        return node->expr_type;
    }
}

inline static ast_node_t *
make_unary_node(parser_t *parser, ast_type_t type, ast_node_t *node, type_info_t type_info)
{
    ast_node_t *unary_node;

    unary_node = f_make_ast_node(parser, type, node, NULL, NULL);

    /* the type may depent on the type of prefix */
    unary_node->expr_type = type_info;

    return unary_node;
}


inline static ast_node_t *
make_literal_node(parser_t *parser, literal_t literal)
{
    ast_node_t *node = f_make_ast_node(parser, AST_LITERAL, NULL, NULL, NULL);
    node->literal    = literal;
    node->value_type = AST_RVALUE;

    return node;
}

/* assumes the symbol already exists and creates a node */
inline static ast_node_t *
make_lvalue_node(parser_t *parser, const token_t *token)
{
    sym_id_t    id;
    ast_node_t *node;

    id = sym_find_id(parser->sym_table, token->hash);

    /* sym will always be defined before we use it here */
    if (id == SYM_ID_NULL)
    {
        syntax_error(token->err_loc, "use of undeclared identifier");
    }

    node             = f_make_ast_node(parser, AST_IDENTIFIER, NULL, NULL, NULL);
    node->sym_id     = id;
    node->value_type = AST_LVALUE;

    return node;
}

void
check_argument_list(parser_t *parser, ast_node_t *func_call)
{
    sym_global_t *func;
    ast_node_t *  argument;
    uint32_t      i;
    type_compat_t compat;

    func = sym_get_global(parser->sym_table, func_call->sym_id);

    argument = func_call->left;

    /* loop through parameters in reverse */
    for (i = func->function.params.size - 1; i >= 0; --i)
    {
        if (argument->type == AST_LIST)
        {
            printf("argument top right %s\n", f_ast_type_to_str(argument->right->type));
            compat = type_compat(argument->right->expr_type, func->function.params.data[i].type);

            if (compat == TYPE_COMPAT_INCOMPAT)
            {
                syntax_error(parser->lexer->last_token.err_loc, "argument doesnt match parameter "
                                                                "type");
            }

            type_print(argument->right->expr_type);
            printf("\n");
            argument = argument->left;
        }

        /* if it's not list AST we must be on the leaf of the tree aka.
		 * the first argument */
        else
        {
            printf("argument top right %s\n", f_ast_type_to_str(argument->type));
            compat = type_compat(argument->expr_type, func->function.params.data[i].type);

            if (compat == TYPE_COMPAT_INCOMPAT)
            {
                syntax_error(parser->lexer->last_token.err_loc, "argument doesnt match parameter "
                                                                "type");
            }

            /* check that parameter count, and the number of arguments match */
            if (i != 0)
            {
                syntax_error(parser->lexer->last_token.err_loc, "number of arguments doesnt match "
                                                                "parameter count");
            }

            break;
        }
    }
}


/* parse function call, curr token must be on identifier */
static ast_node_t *
parse_function_call(parser_t *parser)
{
    sym_global_t *func;
    sym_id_t      id;

    ast_node_t *func_call;

    token_t token = parser->lexer->curr_token;

    /* find the symbol */
    id = sym_find_id(parser->sym_table, token.hash);

    /* make sure that it's declared */
    if (id == SYM_ID_NULL)
    {
        syntax_error(token.err_loc, "undefined function");
    }

    /* get entry */
    func = sym_get_global(parser->sym_table, id);

    /* make sure its a function */
    if (func->kind != SYM_GLOBAL_KIND_FUNCTION)
    {
        syntax_error(token.err_loc, "cannot call variabel");
    }

    func_call         = f_make_ast_node(parser, AST_FUNCTION_CALL, NULL, NULL, NULL);
    func_call->sym_id = id;

    /* skip identifier and '(' */
    f_next_token(parser->lexer);
    f_next_token(parser->lexer);

    func_call->left = f_parse_expression(parser, 0, TOK_PAREN_CLOSED);


    return func_call;
}

static ast_node_t *
make_postfix_operator(parser_t *parser, ast_type_t type, token_t primary_token)
{
    ast_node_t *node;
    type_info_t type_info = NULL_TYPE_INFO;

    if (is_lvalue(primary_token.type))
    {
        node      = make_lvalue_node(parser, &primary_token);
        type_info = sym_get_type_info(parser->sym_table, node->sym_id);
    }
    else if (is_rvalue(primary_token.type))
    {
        syntax_error(parser->lexer->curr_token.err_loc, "rvalues are not assignable");
    }
    else
    {
        syntax_error(parser->lexer->curr_token.err_loc, "unexpected operand "
                                                        "for postfix operator");
    }

    node            = f_make_ast_node(parser, type, NULL, NULL, NULL);
    node->expr_type = type_info;
    return node;
}

/* peek at next token, and determine if it's a postfix, and
 * if it is we generate the ast tree, and skip the postfix,
 * otherwise return NULL */
static ast_node_t *
parse_postfix(parser_t *parser)
{
    token_t primary_token = parser->lexer->last_token;
    token_t postfix_token = parser->lexer->curr_token;

    switch (postfix_token.type)
    {
    case TOK_PAREN_OPEN:
        return parse_function_call(parser);
    case TOK_INCREASE:
        f_next_token(parser->lexer);
        return make_postfix_operator(parser, AST_POST_INCREMENT, primary_token);
    case TOK_DECREASE:
        f_next_token(parser->lexer);
        return make_postfix_operator(parser, AST_POST_DECREMENT, primary_token);
    case TOK_DOT:
        assert(false);
        return NULL;
    case TOK_ARROW:
        assert(false);
        return NULL;
    case TOK_BRACKET_OPEN:
        assert(false);
        return NULL;
    default:
        if (is_lvalue(primary_token.type))
        {
            return make_lvalue_node(parser, &primary_token);
        }
        else if (is_rvalue(primary_token.type))
        {
            return make_literal_node(parser, primary_token.literal);
        }
        else
        {
            syntax_error(parser->lexer->last_token.err_loc, "expected literal or identifier");
        }

        return NULL;
    }
}

static ast_node_t *
make_pre_x_crement_expression(parser_t *parser, ast_type_t prefix_type, ast_node_t *primary)
{
    ast_node_t *expr;

    printf("primary ast type %s\n", f_ast_type_to_str(primary->type));
    if (primary->value_type == AST_RVALUE)
    {
        syntax_error(parser->lexer->last_token.err_loc, "r-value is not "
                                                        "assignable");
    }

    expr            = f_make_ast_node(parser, prefix_type, primary, NULL, NULL);
    expr->expr_type = ast_get_type_info(parser, primary);
    return expr;
}

static ast_node_t *
make_addressof_expression(parser_t *parser, ast_node_t *primary)
{
    ast_node_t *expr;
    type_info_t type;

    if (primary->value_type == AST_RVALUE)
    {
        syntax_error(parser->lexer->last_token.err_loc, "cannot take address of an rvalue");
    }

    expr = f_make_ast_node(parser, AST_ADDRESS, primary, NULL, NULL);
    type = ast_get_type_info(parser, primary);
    ++type.indirection;
    expr->expr_type = type;
	expr->value_type = AST_LVALUE;
    return expr;
}

static ast_node_t *
make_dereference_expression(parser_t *parser, ast_node_t *primary)
{
    ast_node_t *expr;
    type_info_t type;

    if (primary->value_type == AST_RVALUE)
    {
        syntax_error(parser->lexer->last_token.err_loc, "cannot dereference an rvalue");
    }

    expr = f_make_ast_node(parser, AST_ADDRESS, primary, NULL, NULL);
    type = ast_get_type_info(parser, primary);

    /* if it's not a pointer */
    if (!type.indirection)
    {
        syntax_error(parser->lexer->last_token.err_loc, "cannot dereference a non-pointer");
    }

    --type.indirection;
    expr->expr_type = type;
	expr->value_type = AST_LVALUE;
    return expr;
}

static ast_node_t *
make_prefix_expression(parser_t *parser, ast_type_t prefix_type, ast_node_t *primary)
{
	ast_node_t *expr;
	
    switch (prefix_type)
    {
    case AST_PRE_INCREMENT:
        expr = make_pre_x_crement_expression(parser, AST_PRE_INCREMENT, primary);

    case AST_PRE_DECREMENT:
        expr = make_pre_x_crement_expression(parser, AST_PRE_INCREMENT, primary);

    case AST_UNARY_PLUS:
		expr = f_make_ast_node(parser, AST_UNARY_PLUS, primary, NULL, NULL);
		expr->value_type = AST_RVALUE;
		expr->expr_type = ast_get_type_info(parser, primary);		

		/* is always signed */
		expr->expr_type.spec &= ~TYPE_SPEC_UNSIGNED;
		expr->expr_type.spec |=  TYPE_SPEC_SIGNED;
        break;

    case AST_UNARY_MINUS:
		expr = f_make_ast_node(parser, AST_UNARY_MINUS, primary, NULL, NULL);
		expr->value_type = AST_RVALUE;
		expr->expr_type = ast_get_type_info(parser, primary);		

		/* is always signed */
		expr->expr_type.spec &= ~TYPE_SPEC_UNSIGNED;
		expr->expr_type.spec |=  TYPE_SPEC_SIGNED;
        break;

    case AST_UNARY_NOT:
        assert(false);
        return NULL;

    case AST_BIT_NOT:
        assert(false);
        return NULL;

    case AST_DEREF:
        expr = make_dereference_expression(parser, primary);

    case AST_ADDRESS:
        expr = make_addressof_expression(parser, primary);

    default:
        assert(false);
    }

	return expr;
}

static ast_type_t
prefix_token_to_ast(token_type_t token_type)
{
    switch (token_type)
    {
    case TOK_INCREASE:
        return AST_PRE_INCREMENT;
    case TOK_DECREASE:
        return AST_PRE_DECREMENT;
    case TOK_PLUS:
        return AST_UNARY_PLUS;
    case TOK_MINUS:
        return AST_UNARY_MINUS;
    case TOK_NOT_BIT:
        return AST_BIT_NOT;
    case TOK_NOT:
        return AST_UNARY_NOT;
    case TOK_AMPERSAND:
        return AST_ADDRESS;
    case TOK_STAR:
        return AST_DEREF;
    default:
        return AST_NULL;
    }
}

static ast_node_t *
parse_primary_factor(parser_t *parser)
{
    token_t token;

    ast_type_t  prefix_type;
    ast_node_t *postfix_node;

    ast_node_t *tree;

    token = parser->lexer->curr_token;

    if (token.type == TOK_PAREN_OPEN)
    {
        f_next_token(parser->lexer);
        tree = f_parse_expression(parser, 0, TOK_PAREN_CLOSED);
    }
    else
    {
        prefix_type = prefix_token_to_ast(token.type);

        if (prefix_type)
        {
            f_next_token(parser->lexer);
        }

        /* skip primary factor */
        f_next_token(parser->lexer);

        /* also make a node if there isnt a postfix */
        postfix_node = parse_postfix(parser);

        if (prefix_type)
        {
            tree = make_prefix_expression(parser, prefix_type, postfix_node);
        }
        else
        {
            tree = postfix_node;
        }
    }

    printf("exited parser primary factor with current token of type %s\n",
           tok_debug_str(parser->lexer->curr_token.type));

    return tree;
}


typedef struct operator_info
{
    int32_t precedence;

    enum
    {
        ASSOCIAVITY_RIGHT_TO_LEFT,
        ASSOCIAVITY_LEFT_TO_RIGHT

    } associavity;

} operator_info_t;


static const operator_info_t operator_info[_AST_COUNT] =
{
	[AST_LIST] =
	{
		.precedence = 1,
		.associavity = ASSOCIAVITY_LEFT_TO_RIGHT,
	},

	/* assignment expressions */
	[AST_ASSIGN] =
	{
		.precedence = 2,
		.associavity = ASSOCIAVITY_RIGHT_TO_LEFT,
	},

	/* additive expressions */
	[AST_ADD] =
	{
		.precedence = 3,
		.associavity = ASSOCIAVITY_LEFT_TO_RIGHT,
	},

	[AST_MIN] =
	{
		.precedence = 3,
		.associavity = ASSOCIAVITY_LEFT_TO_RIGHT,
	},

	/* multiplicative expressions */
	[AST_MUL] =
	{
		.precedence = 4,
		.associavity = ASSOCIAVITY_LEFT_TO_RIGHT,
	},

	[AST_DIV] =
	{
		.precedence = 4,
		.associavity = ASSOCIAVITY_LEFT_TO_RIGHT,
	},

	[AST_MOD] =
	{
		.precedence = 4,
		.associavity = ASSOCIAVITY_LEFT_TO_RIGHT,
	},

	/* relational expression */
	[AST_EQUAL] =
	{
		.precedence = 5,
		.associavity = ASSOCIAVITY_LEFT_TO_RIGHT,
	},

	[AST_NOT_EQUAL] =
	{
		.precedence = 5,
		.associavity = ASSOCIAVITY_LEFT_TO_RIGHT,
	},

	/* comparative expression */
	[AST_LESSER] =
	{
		.precedence = 6,
		.associavity = ASSOCIAVITY_LEFT_TO_RIGHT,
	},

	[AST_GREATER] =
	{
		.precedence = 6,
		.associavity = ASSOCIAVITY_LEFT_TO_RIGHT,
	},

	[AST_GREATER_EQUAL] =
	{
		.precedence = 6,
		.associavity = ASSOCIAVITY_LEFT_TO_RIGHT,
	},

	[AST_LESSER_EQUAL] =
	{
		.precedence = 6,
		.associavity = ASSOCIAVITY_LEFT_TO_RIGHT,
	},
};

/* adapt types if required, or throw error if not compat */
static void
expr_type_check(parser_t *parser, ast_node_t **left, ast_node_t **right)
{
    type_compat_t compat;

    /* we fetch the compatibility of the righ and left nodes */
    compat = type_compat(ast_get_type_info(parser, *left), ast_get_type_info(parser, *right));

    /* we lookup what to do depent in compat */
    switch (compat)
    {
    /* if the types a compatible we dont need to do anything */
    case TYPE_COMPAT_COMPAT:
        return;

    /* we promote the left or the right */
    case TYPE_COMPAT_PROMOTE_LEFT:
        *left = f_make_ast_node(parser, AST_PROMOTE, *left, NULL, NULL);
        break;

    case TYPE_COMPAT_PROMOTE_RIGHT:
        *right = f_make_ast_node(parser, AST_PROMOTE, *right, NULL, NULL);
        break;

    /* if they are incompatible we throw a syntax error */
    case TYPE_COMPAT_INCOMPAT:
        printf("expr type incompat report\n");
        printf("left type info:\n");
        type_print(ast_get_type_info(parser, *left));
        printf("\n");

        printf("righ type info:\n");
        type_print(ast_get_type_info(parser, *right));
        printf("\n");

        syntax_error(parser->lexer->curr_token.err_loc, "type incompatible in "
                                                        "expression");
        break;
    }
}

/* recursive pratt parser */
ast_node_t *
f_parse_expression(parser_t *parser, int32_t prev_prec, token_type_t terminator)
{
    token_t token;
    token_t op_token;

    ast_node_t *left;
    ast_node_t *right;
    ast_node_t *tmp;

    ast_type_t      op_type;
    type_info_t     expr_type;
    operator_info_t op_info;

    left = parse_primary_factor(parser);

    /* the type of the expression is the type of
	 * the left node */
    expr_type = ast_get_type_info(parser, left);

    op_token = parser->lexer->curr_token;

    if (op_token.type == terminator)
    {
        return left;
    }

    op_type = op_tok_to_ast(op_token.type);

    if (!op_type)
    {
        syntax_error(err_loc(parser->lexer), "expected expression1");
    }

    op_info = operator_info[op_type];

    /* if the prev token precedence is lower than the prev token
	  * we return (go up a level in the tree) */
    while (prev_prec < op_info.precedence ||
           (op_info.associavity == ASSOCIAVITY_RIGHT_TO_LEFT && prev_prec == op_info.precedence))
    {
        f_next_token(parser->lexer);
        right = f_parse_expression(parser, op_info.associavity, terminator);

        /* check type compat, and modify the type required */
        expr_type_check(parser, &left, &right);

        token = parser->lexer->curr_token;

        if (op_info.associavity == ASSOCIAVITY_RIGHT_TO_LEFT)
        {
            tmp   = left;
            left  = right;
            right = tmp;
        }

        left = f_make_ast_node(parser, op_type, left, NULL, right);

        /* @todo: make the type dependent on more than the primary factor */
        left->expr_type  = expr_type;
        left->value_type = AST_RVALUE;

        /* we set the type of the expression */
        if (token.type == terminator)
        {
            return left;
        }

        op_type = op_tok_to_ast(token.type);
        if (!op_type)
        {
            syntax_error(parser->lexer->curr_token.err_loc, "expected "
                                                            "expression");
        }

        op_info = operator_info[op_type];
    }

    return left;
}
