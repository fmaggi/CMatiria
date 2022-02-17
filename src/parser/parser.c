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

static void* allocate_expr(struct mtr_parser* parser, enum mtr_expr_type type) {
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

// ======================= EXPR ================================

static struct mtr_expr* parse_expression(struct mtr_parser* parser);

static struct mtr_expr* parse_primary(struct mtr_parser* parser) {
    if (CHECK(MTR_TOKEN_STRING) || CHECK(MTR_TOKEN_INT) || CHECK(MTR_TOKEN_FLOAT) || CHECK(MTR_TOKEN_TRUE) || CHECK(MTR_TOKEN_FALSE)) {
        struct mtr_literal* node = allocate_expr(parser, MTR_EXPR_LITERAL);

        node->token = advance(parser);

        return (struct mtr_expr*) node;

    } else if (CHECK(MTR_TOKEN_PAREN_L)) {
        struct mtr_grouping* node = allocate_expr(parser, MTR_EXPR_GROUPING);
        advance(parser);

        node->expression = parse_expression(parser);

        consume(parser, MTR_TOKEN_PAREN_R, "Expected ) after expression.");
        return (struct mtr_expr*) node;
    }

    parser_error(parser, "Expected an expression.");

    return NULL;
}

static struct mtr_expr* parse_unary(struct mtr_parser* parser) {
    if (CHECK(MTR_TOKEN_BANG) || CHECK(MTR_TOKEN_MINUS)) {
        struct mtr_unary* node = allocate_expr(parser, MTR_EXPR_UNARY);

        node->operator = advance(parser);
        node->right = parse_unary(parser);
        return (struct mtr_expr*)node;
    }

    return parse_primary(parser);
}

static struct mtr_expr* parse_factor(struct mtr_parser* parser) {
    struct mtr_expr* left = parse_unary(parser);

    while (CHECK(MTR_TOKEN_STAR) || CHECK(MTR_TOKEN_SLASH)) {
        struct mtr_binary* node = allocate_expr(parser, MTR_EXPR_BINARY);

        node->left = left;
        node->operator = advance(parser);;
        node->right = parse_unary(parser);;
        left = (struct mtr_expr*) node;
    }

    return left;
}

static struct mtr_expr* parse_term(struct mtr_parser* parser) {
    struct mtr_expr* left = parse_factor(parser);

    while (CHECK(MTR_TOKEN_PLUS) || CHECK(MTR_TOKEN_MINUS)) {
        struct mtr_binary* node = allocate_expr(parser, MTR_EXPR_BINARY);

        node->left = left;
        node->operator = advance(parser);
        node->right = parse_factor(parser);
        left = (struct mtr_expr*) node;
    }

    return left;
}

static struct mtr_expr* parse_comparison(struct mtr_parser* parser) {
    struct mtr_expr* left = parse_term(parser);

    while (CHECK(MTR_TOKEN_LESS) || CHECK(MTR_TOKEN_LESS_EQUAL) || CHECK(MTR_TOKEN_GREATER) || CHECK(MTR_TOKEN_GREATER_EQUAL)) {
        struct mtr_binary* node = allocate_expr(parser, MTR_EXPR_BINARY);

        node->left = left;
        node->operator = advance(parser);
        node->right = parse_term(parser);
        left = (struct mtr_expr*) node;
    }

    return left;
}

static struct mtr_expr* parse_equality(struct mtr_parser* parser) {
    struct mtr_expr* left = parse_comparison(parser);

    while (CHECK(MTR_TOKEN_BANG_EQUAL) || CHECK(MTR_TOKEN_EQUAL_EQUAL)) {
        struct mtr_binary* node = allocate_expr(parser, MTR_EXPR_BINARY);

        node->left = left;
        node->operator = advance(parser);
        node->right = parse_comparison(parser);
        left = (struct mtr_expr*) node;
    }

    return left;
}

static struct mtr_expr* parse_expression(struct mtr_parser* parser) {
    return parse_equality(parser);
}

// ========================================================================

// ========================= STMTS ========================================


static struct mtr_stmt* parse_statement(struct mtr_parser* parser) {
    struct mtr_expr* expr = parse_expression(parser);
    consume(parser, MTR_TOKEN_SEMICOLON, "Expected ; after expression");

    struct mtr_stmt* stmt = allocate_stmt(parser);
    stmt->expression = expr;
    return stmt;
}

// ========================================================================

struct mtr_stmt* mtr_parse(struct mtr_parser* parser) {
    advance(parser);
    return parse_statement(parser);
}

// ======================= DEBUG =========================

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
