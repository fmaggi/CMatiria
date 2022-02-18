#include "parser.h"

#include "core/log.h"
#include "core/types.h"

#include "error.h"

#include <stdio.h>
#include <stdlib.h>

struct mtr_parser mtr_parser_init(struct mtr_scanner scanner) {
    struct mtr_parser parser = {
        .scanner = scanner,
        .had_error = false
    };

    return parser;
}

static void* allocate_expr(enum mtr_expr_type type) {
    struct mtr_expr* node = malloc(sizeof(struct mtr_expr));
    node->type = type;
    return node;
}

static void* allocate_stmt(struct mtr_parser* parser) {
    struct mtr_stmt* node = malloc(sizeof(struct mtr_stmt));
    return node;
}

#define CHECK(token_type) parser->token.type == token_type

static void parser_error(struct mtr_parser* parser, const char* message) {
    parser->had_error = true;
    mtr_report_error(parser->token, message);
}

static struct mtr_token advance(struct mtr_parser* parser) {
    struct mtr_token previous = parser->token;

    parser->token = mtr_next_token(&parser->scanner);
    while (parser->token.type == MTR_TOKEN_INVALID) {
        parser_error(parser, "Invalid token.");
        parser->token = mtr_next_token(&parser->scanner);
    }

    return previous;
}

static void consume(struct mtr_parser* parser, enum mtr_token_type token, const char* message) {
    if (parser->token.type == token) {
        advance(parser);
        return;
    }
    parser_error(parser, message);
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
static struct mtr_expr* literal(struct mtr_parser* parser, struct mtr_token literal);

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
    [MTR_TOKEN_EQUAL] = { NO_OP },
    [MTR_TOKEN_GREATER] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_COMPARISON },
    [MTR_TOKEN_LESS] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_COMPARISON },
    [MTR_TOKEN_ARROW] = { NO_OP },
    [MTR_TOKEN_BANG_EQUAL] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_EQUALITY },
    [MTR_TOKEN_EQUAL_EQUAL] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_EQUALITY },
    [MTR_TOKEN_GREATER_EQUAL] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_COMPARISON },
    [MTR_TOKEN_LESS_EQUAL] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_COMPARISON },
    [MTR_TOKEN_DOUBLE_SLASH] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_FACTOR },
    [MTR_TOKEN_STRING] = { .prefix = literal, .infix = NULL, .precedence = MTR_PRECEDENCE_NONE },
    [MTR_TOKEN_INT] = { .prefix = literal, .infix = NULL, .precedence = MTR_PRECEDENCE_NONE },
    [MTR_TOKEN_FLOAT] = { .prefix = literal, .infix = NULL, .precedence = MTR_PRECEDENCE_NONE },
    [MTR_TOKEN_AND] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_LOGIC },
    [MTR_TOKEN_OR] = { .prefix = NULL, .infix = binary, .precedence = MTR_PRECEDENCE_LOGIC },
    [MTR_TOKEN_STRUCT] = { NO_OP },
    [MTR_TOKEN_IF] = { NO_OP },
    [MTR_TOKEN_ELSE] = { NO_OP },
    [MTR_TOKEN_TRUE] = { .prefix = literal, .infix = NULL, .precedence = MTR_PRECEDENCE_NONE },
    [MTR_TOKEN_FALSE] = { .prefix = literal, .infix = NULL, .precedence = MTR_PRECEDENCE_NONE },
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
    [MTR_TOKEN_IDENTIFIER] = { NO_OP },
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
    node->operator = op;
    node->right = expression(parser);
    return (struct mtr_expr*) node;
}

static struct mtr_expr* binary(struct mtr_parser* parser, struct mtr_token op, struct mtr_expr* left) {
    struct mtr_binary* node = allocate_expr(MTR_EXPR_BINARY);
    node->left = left;
    node->operator = op;
    node->right = parse_precedence(parser, rules[op.type].precedence + 1);
    return (struct mtr_expr*) node;
}

static struct mtr_expr* grouping(struct mtr_parser* parser, struct mtr_token token) {
    struct mtr_grouping* node = allocate_expr(MTR_EXPR_GROUPING);
    node->expression = expression(parser);
    consume(parser, MTR_TOKEN_PAREN_R, "Expected ) after expression");
    return (struct mtr_expr*) node;
}

static struct mtr_expr* literal(struct mtr_parser* parser, struct mtr_token literal) {
    struct mtr_literal* node = allocate_expr(MTR_EXPR_LITERAL);
    node->token = literal;
    return (struct mtr_expr*) node;
}

static struct mtr_expr* expression(struct mtr_parser* parser) {
    return parse_precedence(parser, MTR_PRECEDENCE_ASSIGN);
}

// ========================================================================

struct mtr_stmt* mtr_parse(struct mtr_parser* parser) {
    advance(parser);
    struct mtr_stmt* stmt = allocate_stmt(parser);
    stmt->expression = expression(parser);
    return stmt;
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

static void free_literal(struct mtr_literal* node) {
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
    case MTR_EXPR_BINARY:   return free_binary(&node->binary);
    case MTR_EXPR_GROUPING: return free_grouping(&node->grouping);
    case MTR_EXPR_LITERAL:  return free_literal(&node->literal);
    case MTR_EXPR_UNARY:    return free_unary(&node->unary);
    }
}

// ======================= DEBUG =========================================

static void print_expr(struct mtr_expr* parser);

static void print_literal(struct mtr_literal* node) {
    printf("%.*s ", (u32)node->token.length, node->token.start);
}

static void print_unary(struct mtr_unary* node) {
    printf("%.*s", (u32)node->operator.length, node->operator.start);
    print_expr(node->right);
}

static void print_binary(struct mtr_binary* node) {
    printf("(%.*s ", (u32)node->operator.length, node->operator.start);
    print_expr(node->left);
    print_expr(node->right);
    printf(")");
}

static void print_grouping(struct mtr_grouping* node) {
    print_expr(node->expression);
}

static void print_expr(struct mtr_expr* node) {
    switch (node->type)
    {
    case MTR_EXPR_LITERAL:
        print_literal((struct mtr_literal*) node);
        break;
    case MTR_EXPR_BINARY:
        print_binary((struct mtr_binary*) node);
        break;
    case MTR_EXPR_GROUPING:
        print_grouping((struct mtr_grouping*) node);
        break;
    case MTR_EXPR_UNARY:
        print_unary((struct mtr_unary*) node);
        break;
    }
}

void mtr_print_expr(struct mtr_expr* node) {
    MTR_LOG_DEBUG("Expression: ");
    print_expr(node);
    putc('\n', stdout);
}

#undef CHECK
