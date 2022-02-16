#include "parser.h"

#include "core/log.h"
#include "core/types.h"

#include <stdio.h>
#include <stdlib.h>

struct mtr_parser mtr_parser_init(struct mtr_scanner scanner) {
    struct mtr_parser parser = {
        .scanner = scanner,
        .array = mtr_new_expr_array()
    };

    return parser;
}

void mtr_parser_shutdown(struct mtr_parser* parser) {
    mtr_delete_expr_array(&parser->array);
}

static void* allocate_node(struct mtr_parser* parser, enum mtr_expr_type type) {
    struct mtr_expr node = {
        .type = type
    };
    return mtr_write_expr(&parser->array, node);
}

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
    struct mtr_literal* node = allocate_node(parser, MTR_EXPR_LITERAL);

    if (CHECK(MTR_TOKEN_STRING) || CHECK(MTR_TOKEN_INT) || CHECK(MTR_TOKEN_FLOAT) || CHECK(MTR_TOKEN_TRUE) || CHECK(MTR_TOKEN_FALSE)) {
        struct mtr_token t = advance(parser);
        node->token = t;
        return (struct mtr_expr*) node;
    }

    parser_error(parser->token, "Expected an expression");

    return (struct mtr_expr*) node;
}

static struct mtr_expr* parse_unary(struct mtr_parser* parser) {
    if (CHECK(MTR_TOKEN_BANG) || CHECK(MTR_TOKEN_MINUS)) {
        struct mtr_token op = advance(parser);

        struct mtr_unary* node = allocate_node(parser, MTR_EXPR_UNARY);

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

        struct mtr_binary* node = allocate_node(parser, MTR_EXPR_BINARY);
        struct mtr_expr* right = parse_unary(parser);

        node->left = left;
        node->operator = op;
        node->right = right;
        left = (struct mtr_expr*) node;
    }

    return left;
}

static struct mtr_expr* parse_term(struct mtr_parser* parser) {
    struct mtr_expr* left = parse_factor(parser);

    while (CHECK(MTR_TOKEN_PLUS) || CHECK(MTR_TOKEN_MINUS)) {
        struct mtr_token op = advance(parser);

        struct mtr_binary* node = allocate_node(parser, MTR_EXPR_BINARY);
        struct mtr_expr* right = parse_factor(parser);

        node->left = left;
        node->operator = op;
        node->right = right;
        left = (struct mtr_expr*) node;
    }

    return left;
}

static struct mtr_expr* parse_comparison(struct mtr_parser* parser) {
    struct mtr_expr* left = parse_term(parser);

    while (CHECK(MTR_TOKEN_LESS) || CHECK(MTR_TOKEN_LESS_EQUAL) || CHECK(MTR_TOKEN_GREATER) || CHECK(MTR_TOKEN_GREATER_EQUAL)) {
        struct mtr_token op = advance(parser);

        struct mtr_binary* node = allocate_node(parser, MTR_EXPR_BINARY);
        struct mtr_expr* right = parse_term(parser);

        node->left = left;
        node->operator = op;
        node->right = right;
        left = (struct mtr_expr*) node;
    }

    return left;
}

static struct mtr_expr* parse_equality(struct mtr_parser* parser) {
    struct mtr_expr* left = parse_comparison(parser);

    while (CHECK(MTR_TOKEN_BANG_EQUAL) || CHECK(MTR_TOKEN_EQUAL_EQUAL)) {
        struct mtr_token op = parser->token;
        advance(parser);

        struct mtr_binary* node = allocate_node(parser, MTR_EXPR_BINARY);
        struct mtr_expr* right = parse_comparison(parser);

        node->left = left;
        node->operator = op;
        node->right = right;
        left = (struct mtr_expr*) node;
    }

    return left;
}

static struct mtr_expr* parse_expression(struct mtr_parser* parser) {
    return parse_equality(parser);
}

struct mtr_expr* mtr_parse(struct mtr_parser* parser) {
    advance(parser);
    return parse_expression(parser);
}

#undef CHECK
