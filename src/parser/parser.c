#include "parser.h"

#include "core/log.h"
#include "core/types.h"

#include "error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void* allocate_expr(enum mtr_expr_type type) {
    struct mtr_expr* node = malloc(sizeof(struct mtr_expr));
    node->type = type;
    return node;
}

static void* allocate_stmt(enum mtr_stmt_type type) {
    struct mtr_stmt* node = malloc(sizeof(struct mtr_stmt));
    node->type = type;
    return node;
}

static void parser_error(struct mtr_parser* parser, const char* message) {
    parser->had_error = true;
    mtr_report_error(parser->token, message);
}

#define CHECK(token_type) (parser->token.type == token_type)

static struct mtr_token advance(struct mtr_parser* parser) {
    struct mtr_token previous = parser->token;

    parser->token = mtr_next_token(&parser->scanner);

    while (CHECK(MTR_TOKEN_INVALID)) {
        parser_error(parser, "Invalid token.");
        parser->token = mtr_next_token(&parser->scanner);
    }

    return previous;
}

static void skip_newline_and_comments(struct mtr_parser* parser) {
    while (CHECK(MTR_TOKEN_NEWLINE) || CHECK(MTR_TOKEN_COMMENT))
        advance(parser);
}

static struct mtr_token consume(struct mtr_parser* parser, enum mtr_token_type token, const char* message) {
    if (parser->token.type == token)
        return advance(parser);

    parser_error(parser, message);
    return invalid_token;
}

static struct mtr_token consume_type(struct mtr_parser* parser) {
    if (parser->token.type >= MTR_TOKEN_U8 && parser->token.type <= MTR_TOKEN_BOOL)
        return advance(parser);

    parser_error(parser, "Expected type.");
    return invalid_token;
}

struct mtr_parser mtr_parser_init(struct mtr_scanner scanner) {
    struct mtr_parser parser = {
        .scanner = scanner,
        .had_error = false
    };

    return parser;
}

// ======================== EXPR =============================

static struct mtr_expr* expression(struct mtr_parser* parser);

enum precedence {
    MTR_PRECEDENCE_NONE,
    MTR_PRECEDENCE_ASSIGN,
    MTR_PRECEDENCE_LOGIC,
    MTR_PRECEDENCE_EQUALITY,
    MTR_PRECEDENCE_COMPARISON,
    MTR_PRECEDENCE_TERM,
    MTR_PRECEDENCE_FACTOR,
    MTR_PRECEDENCE_UNARY,
    MTR_PRECEDENCE_CALL,
    MTR_PRECEDENCE_PRIMARY
};

typedef struct mtr_expr* (*prefix_fn)(struct mtr_parser*, struct mtr_token);
typedef struct mtr_expr* (*infix_fn)(struct mtr_parser*, struct mtr_token, struct mtr_expr*);

struct parser_rule {
    prefix_fn prefix;
    infix_fn infix;
    enum precedence precedence;
};

static struct mtr_expr* unary(struct mtr_parser* parser, struct mtr_token op);
static struct mtr_expr* binary(struct mtr_parser* parser, struct mtr_token op, struct mtr_expr* left);
static struct mtr_expr* grouping(struct mtr_parser* parser, struct mtr_token token);
static struct mtr_expr* primary(struct mtr_parser* parser, struct mtr_token primary);

#define NO_OP .prefix = NULL, .infix = NULL, .precedence = MTR_PRECEDENCE_NONE

static const struct parser_rule rules[] = {
    [MTR_TOKEN_PLUS] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_TERM },
    [MTR_TOKEN_MINUS] = { .prefix = unary, .infix = binary, .precedence = MTR_PRECEDENCE_TERM },
    [MTR_TOKEN_STAR] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_FACTOR },
    [MTR_TOKEN_SLASH] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_FACTOR },
    [MTR_TOKEN_PERCENT] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_FACTOR },
    [MTR_TOKEN_COMMA] = { .prefix = NULL, .infix = NULL, .precedence = MTR_PRECEDENCE_NONE },
    [MTR_TOKEN_COLON] = { .prefix = NULL, .infix = NULL, .precedence = MTR_PRECEDENCE_NONE },
    [MTR_TOKEN_SEMICOLON] = { .prefix = NULL, .infix = NULL, .precedence = MTR_PRECEDENCE_NONE },
    [MTR_TOKEN_DOT] = { .prefix = NULL, .infix = NULL, .precedence = MTR_PRECEDENCE_NONE },
    [MTR_TOKEN_PAREN_L] = { .prefix = grouping, .infix = NULL, .precedence = MTR_PRECEDENCE_NONE },
    [MTR_TOKEN_PAREN_R] = { NO_OP },
    [MTR_TOKEN_SQR_L] = { NO_OP },
    [MTR_TOKEN_SQR_R] = { NO_OP },
    [MTR_TOKEN_CURLY_L] = { NO_OP },
    [MTR_TOKEN_CURLY_R] = { NO_OP },
    [MTR_TOKEN_BANG] = { .prefix = unary, .infix = NULL, .precedence = MTR_PRECEDENCE_UNARY },
    [MTR_TOKEN_EQUAL] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_ASSIGN }, // not sure about this one;
    [MTR_TOKEN_GREATER] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_COMPARISON },
    [MTR_TOKEN_LESS] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_COMPARISON },
    [MTR_TOKEN_ARROW] = { NO_OP },
    [MTR_TOKEN_BANG_EQUAL] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_EQUALITY },
    [MTR_TOKEN_EQUAL_EQUAL] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_EQUALITY },
    [MTR_TOKEN_GREATER_EQUAL] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_COMPARISON },
    [MTR_TOKEN_LESS_EQUAL] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_COMPARISON },
    [MTR_TOKEN_DOUBLE_SLASH] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_FACTOR },
    [MTR_TOKEN_STRING] = { .prefix = primary, .infix = NULL, .precedence = MTR_PRECEDENCE_NONE },
    [MTR_TOKEN_INT] = { .prefix = primary, .infix = NULL, .precedence = MTR_PRECEDENCE_NONE },
    [MTR_TOKEN_FLOAT] = { .prefix = primary, .infix = NULL, .precedence = MTR_PRECEDENCE_NONE },
    [MTR_TOKEN_AND] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_LOGIC },
    [MTR_TOKEN_OR] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_LOGIC },
    [MTR_TOKEN_STRUCT] = { NO_OP },
    [MTR_TOKEN_IF] = { NO_OP },
    [MTR_TOKEN_ELSE] = { NO_OP },
    [MTR_TOKEN_TRUE] = { .prefix = primary, .infix = NULL, .precedence = MTR_PRECEDENCE_NONE },
    [MTR_TOKEN_FALSE] = { .prefix = primary, .infix = NULL, .precedence = MTR_PRECEDENCE_NONE },
    [MTR_TOKEN_FN] = { NO_OP },
    [MTR_TOKEN_RETURN] = { NO_OP },
    [MTR_TOKEN_WHILE] = { NO_OP },
    [MTR_TOKEN_FOR] = { NO_OP },
    [MTR_TOKEN_U8] = { NO_OP },
    [MTR_TOKEN_U16] = { NO_OP },
    [MTR_TOKEN_U32] = { NO_OP },
    [MTR_TOKEN_U64] = { NO_OP },
    [MTR_TOKEN_I8] = { NO_OP },
    [MTR_TOKEN_I16] = { NO_OP },
    [MTR_TOKEN_I32] = { NO_OP },
    [MTR_TOKEN_I64] = { NO_OP },
    [MTR_TOKEN_F32] = { NO_OP },
    [MTR_TOKEN_F64] = { NO_OP },
    [MTR_TOKEN_BOOL] = { NO_OP },
    [MTR_TOKEN_IDENTIFIER] = { .prefix = primary, .infix = NULL, .precedence = MTR_PRECEDENCE_PRIMARY },
    [MTR_TOKEN_NEWLINE] = { NO_OP },
    [MTR_TOKEN_COMMENT] = { NO_OP },
    [MTR_TOKEN_EOF] = { NO_OP },
    [MTR_TOKEN_INVALID] = { NO_OP }
};

#undef NO_OP

static struct mtr_expr* parse_precedence(struct mtr_parser* parser, enum precedence precedece) {
    struct mtr_token token = advance(parser);
    prefix_fn prefix = rules[token.type].prefix;
    if (NULL == prefix) {
        parser_error(parser, "Expected expression.");
        return NULL;
    }

    struct mtr_expr* node = prefix(parser, token);

    while (precedece <= rules[parser->token.type].precedence) {
        struct mtr_token t = advance(parser);
        infix_fn infix = rules[t.type].infix;
        node = infix(parser, t, node);
    }

    return node;
}

static struct mtr_expr* unary(struct mtr_parser* parser, struct mtr_token op) {
    struct mtr_unary* node = allocate_expr(MTR_EXPR_UNARY);
    node->operator = op.type;
    node->right = parse_precedence(parser, rules[op.type].precedence + 1);
    return (struct mtr_expr*) node;
}

static struct mtr_expr* binary(struct mtr_parser* parser, struct mtr_token op, struct mtr_expr* left) {
    struct mtr_binary* node = allocate_expr(MTR_EXPR_BINARY);
    node->left = left;
    node->operator = op.type;
    node->right = parse_precedence(parser, rules[op.type].precedence + 1);
    return (struct mtr_expr*) node;
}

static struct mtr_expr* grouping(struct mtr_parser* parser, struct mtr_token token) {
    struct mtr_grouping* node = allocate_expr(MTR_EXPR_GROUPING);
    node->expression = expression(parser);
    consume(parser, MTR_TOKEN_PAREN_R, "Expected ')'.");
    return (struct mtr_expr*) node;
}

static struct mtr_expr* primary(struct mtr_parser* parser, struct mtr_token primary) {
    struct mtr_primary* node = allocate_expr(MTR_EXPR_PRIMARY);
    node->token = primary;
    return (struct mtr_expr*) node;
}

static struct mtr_expr* expression(struct mtr_parser* parser) {
    return parse_precedence(parser, MTR_PRECEDENCE_ASSIGN);
}

// ========================================================================

static struct mtr_stmt* statement(struct mtr_parser* parser);

static struct mtr_stmt* block(struct mtr_parser* parser) {
    skip_newline_and_comments(parser);

    struct mtr_block* node = allocate_stmt(MTR_STMT_BLOCK);
    node->statements = mtr_new_ast();

    while(!CHECK(MTR_TOKEN_CURLY_R) && !CHECK(MTR_TOKEN_EOF)) {
        struct mtr_stmt* s = statement(parser);
        mtr_write_stmt(&node->statements, s);

        skip_newline_and_comments(parser);
    }

    consume(parser, MTR_TOKEN_CURLY_R, "Expected '}'.");
    return (struct mtr_stmt*) node;
}

static struct mtr_stmt* expr_statement(struct mtr_parser* parser) {
    if (CHECK(MTR_TOKEN_CURLY_L)) {
        advance(parser);
        return block(parser);
    }

    struct mtr_expr_stmt* node = allocate_stmt(MTR_STMT_EXPRESSION);
    node->expression = expression(parser);
    consume(parser, MTR_TOKEN_SEMICOLON, "Expected ';'.");
    return (struct mtr_stmt*)  node;
}

static struct mtr_stmt* func_decl(struct mtr_parser* parser) {
    struct mtr_fn_decl* node = allocate_stmt(MTR_STMT_FUNC);

    advance(parser);

    node->name = consume(parser, MTR_TOKEN_IDENTIFIER, "Expected identifier.");
    consume(parser, MTR_TOKEN_PAREN_L, "Expected '('.");

    u8 argc = 0;
    struct mtr_var_decl vars[255];
    while (argc < 255 && !CHECK(MTR_TOKEN_PAREN_R)) {
        struct mtr_var_decl* var = vars + argc++;
        var->var_type = consume_type(parser).type;
        var->name = consume(parser, MTR_TOKEN_IDENTIFIER, "Expected identifier.");

        if (CHECK(MTR_TOKEN_PAREN_R))
            break;

        consume(parser, MTR_TOKEN_COMMA, "Expected ','.");
    }

    if (argc > 255)
        parser_error(parser, "Exceded maximum number of arguments (255)");

    consume(parser, MTR_TOKEN_PAREN_R, "Expected ')'."); // need to check again in case we broke out of the loop because of arg count

    node->args.argc = argc;

    if (argc > 0) {
        node->args.argv = malloc(sizeof(struct mtr_var_decl) * argc);
        if (NULL == node->args.argv)
            MTR_LOG_ERROR("Bad allocation.");
        else
            memcpy(node->args.argv, vars, sizeof(struct mtr_var_decl) * argc);
    }

    consume(parser, MTR_TOKEN_ARROW, "Expected '->'.");

    node->return_type = consume_type(parser).type;

    skip_newline_and_comments(parser);

    consume(parser, MTR_TOKEN_CURLY_L, "Expected '{'.");

    node->body = block(parser);

    return (struct mtr_stmt*)  node;
}

static struct mtr_stmt* var_decl(struct mtr_parser* parser) {
    struct mtr_var_decl* node = allocate_stmt(MTR_STMT_VAR_DECL);

    node->var_type = advance(parser).type; // because we are here we alredy know its a type!
    node->name = consume(parser, MTR_TOKEN_IDENTIFIER, "Expected identifier.");

    if (CHECK(MTR_TOKEN_EQUAL)) {
        advance(parser);
        node->value = expression(parser);
    }

    consume(parser, MTR_TOKEN_SEMICOLON, "Expected ';'.");

    return (struct mtr_stmt*)  node;
}

static struct mtr_stmt* statement(struct mtr_parser* parser) {
    switch (parser->token.type)
    {
    case MTR_TOKEN_U8:
    case MTR_TOKEN_U16:
    case MTR_TOKEN_U32:
    case MTR_TOKEN_U64:
    case MTR_TOKEN_I8:
    case MTR_TOKEN_I16:
    case MTR_TOKEN_I32:
    case MTR_TOKEN_I64:
    case MTR_TOKEN_F32:
    case MTR_TOKEN_F64:
    case MTR_TOKEN_BOOL:
        return var_decl(parser);
    case MTR_TOKEN_FN:
        return func_decl(parser);
    default:
        break;
    }

    return expr_statement(parser);
}

// ========================================================================

struct mtr_ast mtr_parse(struct mtr_parser* parser) {
    advance(parser);
    skip_newline_and_comments(parser);

    struct mtr_ast ast = mtr_new_ast();

    while (parser->token.type != MTR_TOKEN_EOF) {
        struct mtr_stmt* stmt = statement(parser);
        mtr_write_stmt(&ast, stmt);

        skip_newline_and_comments(parser);
    }

    return ast;
}

// =======================================================================

struct mtr_ast mtr_new_ast() {
    struct mtr_ast ast = {
        .capacity = 8,
        .size = 0,
        .statements = NULL
    };

    void* temp = malloc(sizeof(struct mtr_stmt*) * 8);
    if (NULL == temp) {
        MTR_LOG_ERROR("Bad allocation.");
        return ast;
    }

    ast.statements = temp;
    return ast;
}

void mtr_write_stmt(struct mtr_ast* ast, struct mtr_stmt* statement) {
    if (ast->size == ast->capacity) {
        size_t new_cap = ast->capacity * 2;

        void* temp = malloc(sizeof(struct mtr_stmt*) * new_cap);
        if (NULL == temp) {
            MTR_LOG_ERROR("Bad allocation.");
            return;
        }

        memcpy(temp, ast->statements, sizeof(struct mtr_stmt*) * ast->size);

        free(ast->statements);
        ast->statements = temp;
        ast->capacity = new_cap;
    }

    ast->statements[ast->size++] = statement;
}

// =======================================================================

static void free_binary(struct mtr_binary* node) {
    mtr_free_expr(node->left);
    mtr_free_expr(node->right);
    node->left = NULL;
    node->right = NULL;
    free(node);
}

static void free_grouping(struct mtr_grouping* node) {
    mtr_free_expr(node->expression);
    node->expression = NULL;
    free(node);
}

static void free_primary(struct mtr_primary* node) {
    free(node);
}

static void free_unary(struct mtr_unary* node) {
    mtr_free_expr(node->right);
    node->right = NULL;
    free(node);
}

void mtr_free_expr(struct mtr_expr* node) {
    switch (node->type)
    {
    case MTR_EXPR_BINARY:   return free_binary((struct mtr_binary*) node);
    case MTR_EXPR_GROUPING: return free_grouping((struct mtr_grouping*) node);
    case MTR_EXPR_PRIMARY:  return free_primary((struct mtr_primary*) node);
    case MTR_EXPR_UNARY:    return free_unary((struct mtr_unary*) node);
    }
}

// ======================= DEBUG =========================================

static void print_expr(struct mtr_expr* parser);

static void print_primary(struct mtr_primary* node) {
    MTR_PRINT_DEBUG("%.*s ", (u32)node->token.length, node->token.start);
}

static void print_unary(struct mtr_unary* node) {
    MTR_PRINT_DEBUG("%s", mtr_token_type_to_str(node->operator));
    print_expr(node->right);
}

static void print_binary(struct mtr_binary* node) {
    MTR_PRINT_DEBUG("(%s ", mtr_token_type_to_str(node->operator));
    print_expr(node->left);
    print_expr(node->right);
    MTR_PRINT_DEBUG(")");
}

static void print_grouping(struct mtr_grouping* node) {
    print_expr(node->expression);
}

static void print_expr(struct mtr_expr* node) {
    switch (node->type)
    {
    case MTR_EXPR_PRIMARY:  return print_primary((struct mtr_primary*) node);
    case MTR_EXPR_BINARY:   return print_binary((struct mtr_binary*) node);
    case MTR_EXPR_GROUPING: return print_grouping((struct mtr_grouping*) node);
    case MTR_EXPR_UNARY:    return print_unary((struct mtr_unary*) node);
    }
}

void mtr_print_expr(struct mtr_expr* node) {
    MTR_LOG_DEBUG("Expression: ");
    print_expr(node);
    MTR_PRINT_DEBUG("\n");
}

static void print_stmt(struct mtr_stmt* stmt);

static void print_block(struct mtr_block* block) {
    for (u32 i = 0; i < block->statements.size; ++i) {
        print_stmt(block->statements.statements[i]);
    }
}

static void print_var(struct mtr_var_decl* decl) {

}

static void print_expr_stmt(struct mtr_expr_stmt* decl) {
    mtr_print_expr(decl->expression);
}

static void print_func(struct mtr_fn_decl* decl) {
    MTR_PRINT_DEBUG("function: %.*s(", (u32)decl->name.length, decl->name.start);

    if (decl->args.argc > 0) {
        for (u32 i = 0; i < decl->args.argc - 1; ++i) {
            struct mtr_var_decl param = decl->args.argv[i];
            MTR_PRINT_DEBUG("%s %.*s, ", mtr_token_type_to_str(param.var_type), (u32)param.name.length, param.name.start);
        }

        struct mtr_var_decl param = decl->args.argv[decl->args.argc-1];
        MTR_PRINT_DEBUG("%s %.*s", mtr_token_type_to_str(param.var_type), (u32)param.name.length, param.name.start);
    }

    MTR_PRINT_DEBUG(") -> %s\n", mtr_token_type_to_str(decl->return_type));
    print_stmt(decl->body);
}

static void print_stmt(struct mtr_stmt* decl) {
    switch (decl->type)
    {
    case MTR_STMT_FUNC:       return print_func((struct mtr_fn_decl*) decl);
    case MTR_STMT_EXPRESSION: return print_expr_stmt((struct mtr_expr_stmt*) decl);
    case MTR_STMT_VAR_DECL:   return print_var((struct mtr_var_decl*) decl);
    case MTR_STMT_BLOCK:      return print_block((struct mtr_block*) decl);
    }
}

void mtr_print_stmt(struct mtr_stmt* decl) {
    MTR_LOG_DEBUG("Declaration: ");
    print_stmt(decl);
    MTR_PRINT_DEBUG("\n");
}

#undef CHECK
