#include "parser.h"

#include "core/types.h"

#include "core/report.h"
#include "core/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ALLOCATE_EXPR(type, expr) allocate_expr(type, sizeof(struct expr))

static void* allocate_expr(enum mtr_expr_type type, size_t size) {
    struct mtr_expr* node = malloc(size);
    node->type = type;
    return node;
}

static struct mtr_stmt allocate_stmt(enum mtr_stmt_type type) {
    struct mtr_stmt node;
    node.type = type;
    return node;
}

static void parser_error(struct mtr_parser* parser, const char* message) {
    parser->had_error = true;
    if (!parser->panic)
        mtr_report_error(parser->token, message, parser->scanner.source);
    parser->panic = true;
}

#define CHECK(token_type) (parser->token.type == token_type)

static struct mtr_token advance(struct mtr_parser* parser) {
    struct mtr_token previous = parser->token;

    parser->token = mtr_next_token(&parser->scanner);

    while (CHECK(MTR_TOKEN_COMMENT))
        parser->token = mtr_next_token(&parser->scanner);

    while (CHECK(MTR_TOKEN_INVALID)) {
        parser_error(parser, "Invalid token.");
        parser->token = mtr_next_token(&parser->scanner);
    }

    return previous;
}

static struct mtr_token consume(struct mtr_parser* parser, enum mtr_token_type token, const char* message) {
    if (parser->token.type == token)
        return advance(parser);

    parser_error(parser, message);
    return invalid_token;
}

static struct mtr_token consume_type(struct mtr_parser* parser) {
    if (CHECK(MTR_TOKEN_INT) || CHECK(MTR_TOKEN_FLOAT) || CHECK(MTR_TOKEN_BOOL))
        return advance(parser);

    parser_error(parser, "Expected type.");
    return invalid_token;
}

struct mtr_parser mtr_parser_init(struct mtr_scanner scanner) {
    struct mtr_parser parser = {
        .scanner = scanner,
        .had_error = false,
        .panic = false
    };

    return parser;
}

static void synchronize(struct mtr_parser* parser) {
    if (!parser->panic)
        return;

    parser->panic = false;
    while (!CHECK(MTR_TOKEN_EOF)) {
        switch (parser->token.type)
        {
        case MTR_TOKEN_INT:
        case MTR_TOKEN_FLOAT:
        case MTR_TOKEN_BOOL:
        case MTR_TOKEN_FN:
        case MTR_TOKEN_IF:
        case MTR_TOKEN_WHILE:
        case MTR_TOKEN_CURLY_L:
        case MTR_TOKEN_CURLY_R:
            return;
        default:
            advance(parser);
        }
    }
}

// ======================== EXPR =============================

static struct mtr_expr* expression(struct mtr_parser* parser);

enum precedence {
    NONE,
    LOGIC,
    EQUALITY,
    COMPARISON,
    TERM,
    FACTOR,
    UNARY,
    CALL,
    PRIMARY
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

#define NO_OP .prefix = NULL, .infix = NULL, .precedence = NONE

static const struct parser_rule rules[] = {
    [MTR_TOKEN_PLUS] = { .prefix = NULL, .infix = binary, .precedence = TERM },
    [MTR_TOKEN_MINUS] = { .prefix = unary, .infix = binary, .precedence = TERM },
    [MTR_TOKEN_STAR] = { .prefix = NULL, .infix = binary, .precedence = FACTOR },
    [MTR_TOKEN_SLASH] = { .prefix = NULL, .infix = binary, .precedence = FACTOR },
    [MTR_TOKEN_PERCENT] = { .prefix = NULL, .infix = binary, .precedence = FACTOR },
    [MTR_TOKEN_COMMA] = { .prefix = NULL, .infix = NULL, .precedence = NONE },
    [MTR_TOKEN_COLON] = { .prefix = NULL, .infix = NULL, .precedence = NONE },
    [MTR_TOKEN_SEMICOLON] = { .prefix = NULL, .infix = NULL, .precedence = NONE },
    [MTR_TOKEN_DOT] = { .prefix = NULL, .infix = NULL, .precedence = NONE },
    [MTR_TOKEN_PAREN_L] = { .prefix = grouping, .infix = NULL, .precedence = NONE },
    [MTR_TOKEN_PAREN_R] = { NO_OP },
    [MTR_TOKEN_SQR_L] = { NO_OP },
    [MTR_TOKEN_SQR_R] = { NO_OP },
    [MTR_TOKEN_CURLY_L] = { NO_OP },
    [MTR_TOKEN_CURLY_R] = { NO_OP },
    [MTR_TOKEN_BANG] = { .prefix = unary, .infix = NULL, .precedence = UNARY },
    [MTR_TOKEN_ASSIGN] = { NO_OP },
    [MTR_TOKEN_GREATER] = { .prefix = NULL, .infix = binary, .precedence = COMPARISON },
    [MTR_TOKEN_LESS] = { .prefix = NULL, .infix = binary, .precedence = COMPARISON },
    [MTR_TOKEN_ARROW] = { NO_OP },
    [MTR_TOKEN_BANG_EQUAL] = { .prefix = NULL, .infix = binary, .precedence = EQUALITY },
    [MTR_TOKEN_EQUAL] = { .prefix = NULL, .infix = binary, .precedence = EQUALITY },
    [MTR_TOKEN_GREATER_EQUAL] = { .prefix = NULL, .infix = binary, .precedence = COMPARISON },
    [MTR_TOKEN_LESS_EQUAL] = { .prefix = NULL, .infix = binary, .precedence = COMPARISON },
    [MTR_TOKEN_DOUBLE_SLASH] = { .prefix = NULL, .infix = binary, .precedence = FACTOR },
    [MTR_TOKEN_STRING_LITERAL] = { .prefix = primary, .infix = NULL, .precedence = NONE },
    [MTR_TOKEN_INT_LITERAL] = { .prefix = primary, .infix = NULL, .precedence = NONE },
    [MTR_TOKEN_FLOAT_LITERAL] = { .prefix = primary, .infix = NULL, .precedence = NONE },
    [MTR_TOKEN_AND] = { .prefix = NULL, .infix = binary, .precedence = LOGIC },
    [MTR_TOKEN_OR] = { .prefix = NULL, .infix = binary, .precedence = LOGIC },
    [MTR_TOKEN_STRUCT] = { NO_OP },
    [MTR_TOKEN_IF] = { NO_OP },
    [MTR_TOKEN_ELSE] = { NO_OP },
    [MTR_TOKEN_TRUE] = { .prefix = primary, .infix = NULL, .precedence = NONE },
    [MTR_TOKEN_FALSE] = { .prefix = primary, .infix = NULL, .precedence = NONE },
    [MTR_TOKEN_FN] = { NO_OP },
    [MTR_TOKEN_RETURN] = { NO_OP },
    [MTR_TOKEN_WHILE] = { NO_OP },
    [MTR_TOKEN_FOR] = { NO_OP },
    [MTR_TOKEN_INT] = { NO_OP },
    [MTR_TOKEN_FLOAT] = { NO_OP },
    [MTR_TOKEN_BOOL] = { NO_OP },
    [MTR_TOKEN_IDENTIFIER] = { .prefix = primary, .infix = NULL, .precedence = PRIMARY },
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
    struct mtr_unary* node = ALLOCATE_EXPR(MTR_EXPR_UNARY, mtr_unary);
    node->operator.token = op;
    node->right = parse_precedence(parser, rules[op.type].precedence + 1);
    return (struct mtr_expr*) node;
}

static struct mtr_expr* binary(struct mtr_parser* parser, struct mtr_token op, struct mtr_expr* left) {
    struct mtr_binary* node = ALLOCATE_EXPR(MTR_EXPR_BINARY, mtr_binary);
    node->left = left;
    node->operator.token = op;
    node->right = parse_precedence(parser, rules[op.type].precedence + 1);
    return (struct mtr_expr*) node;
}

static struct mtr_expr* grouping(struct mtr_parser* parser, struct mtr_token token) {
    struct mtr_grouping* node = ALLOCATE_EXPR(MTR_EXPR_GROUPING, mtr_grouping);
    node->expression = expression(parser);
    consume(parser, MTR_TOKEN_PAREN_R, "Expected ')'.");
    return (struct mtr_expr*) node;
}

static struct mtr_expr* primary(struct mtr_parser* parser, struct mtr_token primary) {
    struct mtr_primary* node = ALLOCATE_EXPR(MTR_EXPR_PRIMARY, mtr_primary);
    node->symbol.token = primary;
    return (struct mtr_expr*) node;
}

static struct mtr_expr* expression(struct mtr_parser* parser) {
    return parse_precedence(parser, LOGIC);
}

// ============================ STMT =====================================

static struct mtr_stmt declaration(struct mtr_parser* parser);

static struct mtr_stmt block(struct mtr_parser* parser) {
    struct mtr_stmt stmt = allocate_stmt(MTR_STMT_BLOCK);
    struct mtr_block* node = &stmt.block;
    node->statements = mtr_new_ast();

    consume(parser, MTR_TOKEN_CURLY_L, "Expected '{'.");

    while(!CHECK(MTR_TOKEN_CURLY_R) && !CHECK(MTR_TOKEN_EOF)) {
        struct mtr_stmt s = declaration(parser);
        synchronize(parser);
        mtr_write_stmt(&node->statements, s);
    }

    consume(parser, MTR_TOKEN_CURLY_R, "Expected '}'.");
    return stmt;
}

static struct mtr_stmt if_stmt(struct mtr_parser* parser) {
    struct mtr_stmt stmt = allocate_stmt(MTR_STMT_IF);
    struct mtr_if* node = &stmt.if_s;

    advance(parser);
    consume(parser, MTR_TOKEN_PAREN_L, "Expected '('.");
    node->condition = expression(parser);
    consume(parser, MTR_TOKEN_PAREN_R, "Expected ')'.");

    node->then = block(parser).block;

    struct mtr_ast e = {
        .statements = NULL,
        .size = 0,
        .capacity = 0
    };

    node->otherwise.statements = e;

    if (CHECK(MTR_TOKEN_ELSE)) {
        advance(parser);
        node->otherwise = block(parser).block;
    }

    return stmt;
}

static struct mtr_stmt while_stmt(struct mtr_parser* parser) {
    struct mtr_stmt stmt = allocate_stmt(MTR_STMT_WHILE);
    struct mtr_while* node = &stmt.while_s;

    advance(parser);
    consume(parser, MTR_TOKEN_PAREN_L, "Expected '('.");
    node->condition = expression(parser);
    consume(parser, MTR_TOKEN_PAREN_R, "Expected ')'.");

    node->body = block(parser).block;

    return stmt;
}

static struct mtr_stmt assignment(struct mtr_parser* parser) {
    struct mtr_stmt stmt = allocate_stmt(MTR_STMT_ASSIGNMENT);
    struct mtr_assignment* node = &stmt.assignment;
    node->variable.token = consume(parser, MTR_TOKEN_IDENTIFIER, "Expected a name.");
    consume(parser, MTR_TOKEN_ASSIGN, "Expected ':='.");
    node->expression = expression(parser);
    consume(parser, MTR_TOKEN_SEMICOLON, "Expected ';'.");
    return stmt;
}

static struct mtr_stmt statement(struct mtr_parser* parser) {
    switch (parser->token.type)
    {
    case MTR_TOKEN_IF:      return if_stmt(parser);
    case MTR_TOKEN_WHILE:   return while_stmt(parser);
    case MTR_TOKEN_CURLY_L: return block(parser);
    default:
        return assignment(parser);
    }
}

static struct mtr_stmt func_decl(struct mtr_parser* parser) {
    struct mtr_stmt stmt = allocate_stmt(MTR_STMT_FN);
    struct mtr_function* node = &stmt.function;

    advance(parser);

    node->symbol.token = consume(parser, MTR_TOKEN_IDENTIFIER, "Expected identifier.");
    consume(parser, MTR_TOKEN_PAREN_L, "Expected '('.");

    u32 argc = 0;
    struct mtr_variable vars[255];
    bool cont = true;
    while (argc < 255 && !CHECK(MTR_TOKEN_PAREN_R) && cont) {
        struct mtr_variable* var = vars + argc++;
        var->symbol.type.type = mtr_get_data_type(consume_type(parser).type);
        var->symbol.token = consume(parser, MTR_TOKEN_IDENTIFIER, "Expected identifier.");
        var->value = NULL;

        if (CHECK(MTR_TOKEN_PAREN_R))
            break;

        cont = consume(parser, MTR_TOKEN_COMMA, "Expected ','.").type == MTR_TOKEN_COMMA;
    }

    if (argc > 255)
        parser_error(parser, "Exceded maximum number of arguments (255)");

    consume(parser, MTR_TOKEN_PAREN_R, "Expected ')'."); // need to check again in case we broke out of the loop because of arg count

    node->argc = argc;
    node->argv = NULL;

    if (argc > 0) {
        node->argv = malloc(sizeof(struct mtr_variable) * argc);
        if (NULL == node->argv)
            MTR_LOG_ERROR("Bad allocation.");
        else
            memcpy(node->argv, vars, sizeof(struct mtr_variable) * argc);
    }

    consume(parser, MTR_TOKEN_ARROW, "Expected '->'.");

    node->symbol.type.type = mtr_get_data_type(consume_type(parser).type);
    node->body = block(parser).block;

    return stmt;
}

static struct mtr_stmt variable(struct mtr_parser* parser) {
    struct mtr_stmt stmt = allocate_stmt(MTR_STMT_VAR);
    struct mtr_variable* node = &stmt.variable;

    node->symbol.type.type = mtr_get_data_type(advance(parser).type); // because we are here we alredy know its a type!
    node->symbol.token = consume(parser, MTR_TOKEN_IDENTIFIER, "Expected identifier.");
    node->value = NULL;

    if (CHECK(MTR_TOKEN_ASSIGN)) {
        advance(parser);
        node->value = expression(parser);
    }

    consume(parser, MTR_TOKEN_SEMICOLON, "Expected ';' or ':='.");

    return stmt;
}

static struct mtr_stmt declaration(struct mtr_parser* parser) {
    switch (parser->token.type)
    {
    case MTR_TOKEN_INT:
    case MTR_TOKEN_FLOAT:
    case MTR_TOKEN_BOOL:
        return variable(parser);
    default:
        return statement(parser);
    }
}

static struct mtr_stmt global_declaration(struct mtr_parser* parser) {
    switch (parser->token.type)
    {
    case MTR_TOKEN_FN: return func_decl(parser);
    default:
        break;
    }
    parser_error(parser, "Expected function declaration.");
    exit(-1);
}

#undef CHECK

// ========================================================================

struct mtr_ast mtr_parse(struct mtr_parser* parser) {
    advance(parser);

    struct mtr_ast ast = mtr_new_ast();

    while (parser->token.type != MTR_TOKEN_EOF) {
        struct mtr_stmt stmt = global_declaration(parser);
        synchronize(parser);
        mtr_write_stmt(&ast, stmt);
    }

    return ast;
}

// =======================================================================

#include "core/memory.h"

struct mtr_ast mtr_new_ast() {
    struct mtr_ast ast = {
        .capacity = 8,
        .size = 0,
        .statements = NULL
    };

    void* temp = malloc(sizeof(struct mtr_stmt) * 8);
    if (NULL == temp)
        MTR_LOG_ERROR("Bad allocation.");
    else
        ast.statements = temp;
    return ast;
}

void mtr_write_stmt(struct mtr_ast* ast, struct mtr_stmt statement) {
    if (ast->size == ast->capacity) {
        size_t new_cap = ast->capacity * 2;
        ast->statements = mtr_resize_array(ast->statements, ast->capacity, new_cap, sizeof(struct mtr_stmt));
        if (NULL == ast->statements) {
            ast->capacity = 0;
            return;
        }
        ast->capacity = new_cap;
    }

    ast->statements[ast->size++] = statement;
}

void mtr_delete_ast(struct mtr_ast* ast) {
    for (size_t i = 0; i < ast->size; i++) {
        struct mtr_stmt* s = ast->statements + i;
        switch (s->type)
        {
        case MTR_STMT_BLOCK:
            mtr_delete_ast(&s->block.statements);
            break;
        case MTR_STMT_ASSIGNMENT:
            mtr_free_expr(s->assignment.expression);
            s->assignment.expression = NULL;
            break;
        case MTR_STMT_FN:
            if (s->function.argc > 0)
                free(s->function.argv);
            s->function.argv = NULL;
            mtr_delete_ast(&s->function.body.statements);
            break;
        case MTR_STMT_IF:
            mtr_delete_ast(&s->if_s.then.statements);
            mtr_delete_ast(&s->if_s.otherwise.statements);
            mtr_free_expr(s->if_s.condition);
            break;
        case MTR_STMT_WHILE:
            mtr_delete_ast(&s->while_s.body.statements);
            mtr_free_expr(s->while_s.condition);
            break;
        case MTR_STMT_VAR:
            if (s->variable.value)
                mtr_free_expr(s->variable.value);
            s->variable.value = NULL;
            break;
        }
    }

    free(ast->statements);
    ast->statements = NULL;
    ast->size = 0;
    ast->capacity = 0;
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
