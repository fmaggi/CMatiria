#include "parser.h"

#include "core/log.h"
#include "core/types.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(token_type) parser->token.type == token_type

static void parser_error(struct mtr_token t, const char* message) {
    MTR_LOG_ERROR("%s", message);
}

static struct mtr_token advance(struct mtr_parser* parser) {
    struct mtr_token previous = parser->token;

    parser->token = mtr_next_token(&parser->scanner);
    if (parser->token.type == MTR_TOKEN_INVALID) {
        parser_error(parser->token, "Invalid token");
    }

    return previous;
}

static struct mtr_expr* parse_primary(struct mtr_parser* parser) {
    struct mtr_literal* node = malloc(sizeof(struct mtr_literal));

    node->expr.type = MTR_EXPR_LITERAL;

    struct mtr_token t = parser->token;
    if (t.type >= MTR_TOKEN_STRING && t.type <= MTR_TOKEN_BOOLEAN) {
        advance(parser);
        node->token = t;
        return (struct mtr_expr*) node;
    }

    parser_error(t, "Expected an expression");

    return (struct mtr_expr*) node;
}

static struct mtr_expr* parse_unary(struct mtr_parser* parser) {
    if (CHECK(MTR_TOKEN_BANG) || CHECK(MTR_TOKEN_MINUS)) {
        struct mtr_token op = advance(parser);

        struct mtr_unary* node = malloc(sizeof(struct mtr_unary));

        node->expr.type = MTR_EXPR_UNARY;
        node->operator = op;
        node->right = parse_unary(parser);
        return (struct mtr_expr*)node;
    }

    return parse_primary(parser);
}

static struct mtr_expr* parse_factor(struct mtr_parser* parser) {
    struct mtr_expr* left = parse_unary(parser);

    while (CHECK(MTR_TOKEN_STAR) || CHECK(MTR_TOKEN_SLASH)) {
        struct mtr_token op = advance(parser);

        struct mtr_binary* node = malloc(sizeof(struct mtr_binary));
        struct mtr_expr* right = parse_unary(parser);

        node->left = left;
        node->operator = op;
        node->right = right;
        node->expr.type = MTR_EXPR_BINARY;
        left = (struct mtr_expr*) node;
    }

    return left;
}

static struct mtr_expr* parse_term(struct mtr_parser* parser) {
    struct mtr_expr* left = parse_factor(parser);

    while (CHECK(MTR_TOKEN_PLUS) || CHECK(MTR_TOKEN_MINUS)) {
        struct mtr_token op = advance(parser);

        struct mtr_binary* node = malloc(sizeof(struct mtr_binary));
        struct mtr_expr* right = parse_factor(parser);

        node->left = left;
        node->operator = op;
        node->right = right;
        node->expr.type = MTR_EXPR_BINARY;
        left = (struct mtr_expr*) node;
    }

    return left;
}

static struct mtr_expr* parse_comparison(struct mtr_parser* parser) {
    struct mtr_expr* left = parse_term(parser);

    while (CHECK(MTR_TOKEN_LESS) || CHECK(MTR_TOKEN_LESS_EQUAL) || CHECK(MTR_TOKEN_GREATER) || CHECK(MTR_TOKEN_GREATER_EQUAL)) {
        struct mtr_token op = advance(parser);

        struct mtr_binary* node = malloc(sizeof(struct mtr_binary));
        struct mtr_expr* right = parse_term(parser);

        node->left = left;
        node->operator = op;
        node->right = right;
        node->expr.type = MTR_EXPR_BINARY;
        left = (struct mtr_expr*) node;
    }

    return left;
}

static struct mtr_expr* parse_equality(struct mtr_parser* parser) {
    struct mtr_expr* left = parse_comparison(parser);

    while (CHECK(MTR_TOKEN_BANG_EQUAL) || CHECK(MTR_TOKEN_EQUAL_EQUAL)) {
        struct mtr_token op = parser->token;
        advance(parser);

        struct mtr_binary* node = malloc(sizeof(struct mtr_binary));
        struct mtr_expr* right = parse_comparison(parser);

        node->left = left;
        node->operator = op;
        node->right = right;
        node->expr.type = MTR_EXPR_BINARY;
        left = (struct mtr_expr*) node;
    }

    return left;
}

static struct mtr_expr* parse_expression(struct mtr_parser* parser) {
    return parse_equality(parser);
}

static void print_expr(struct mtr_expr* parser);

struct mtr_expr* mtr_parse(struct mtr_parser* parser) {
    advance(parser);
    return parse_expression(parser);
}

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
        break;
    case MTR_EXPR_UNARY:
        print_unary((struct mtr_unary*) node);
        break;
    }
}

void mtr_print_expr(struct mtr_expr* node) {
    print_expr(node);
    putc('\n', stdout);
}

#undef CHECK
