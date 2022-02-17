#ifndef _MTR_EXPR_H
#define _MTR_EXPR_H

#include "scanner/token.h"

enum mtr_expr_type {
    MTR_EXPR_BINARY,
    MTR_EXPR_LITERAL,
    MTR_EXPR_GROUPING,
    MTR_EXPR_UNARY
};

struct mtr_expr;

struct mtr_unary {
    struct mtr_token operator;
    struct mtr_expr* right;
};

struct mtr_literal {
    struct mtr_token token;
    union {
        u64 integer;
        f64 floating;
        char* string;
    };
};

struct mtr_grouping {
    struct mtr_expr* expression;
};

struct mtr_binary {
    struct mtr_token operator;
    struct mtr_expr* right;
    struct mtr_expr* left;
};

struct mtr_expr {

    union {
        struct mtr_binary binary;
        struct mtr_unary unary;
        struct mtr_literal literal;
        struct mtr_grouping grouping;
    };

    enum mtr_expr_type type;
};

void mtr_print_expr(struct mtr_expr* node);

#endif
