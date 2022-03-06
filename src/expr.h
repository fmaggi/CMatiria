#ifndef MTR_EXPR_H
#define MTR_EXPR_H

#include "token.h"
#include "symbol.h"

enum mtr_expr_type {
    MTR_EXPR_BINARY,
    MTR_EXPR_PRIMARY,
    MTR_EXPR_LITERAL,
    MTR_EXPR_GROUPING,
    MTR_EXPR_UNARY,
    MTR_EXPR_CALL,
    MTR_EXPR_CAST
};

struct mtr_expr {
    enum mtr_expr_type type;
};

struct mtr_cast {
    struct mtr_expr expr_;
    struct mtr_expr* right;
    struct mtr_type to;
};

struct mtr_unary {
    struct mtr_expr expr_;
    struct mtr_expr* right;
    struct mtr_symbol operator;
};

struct mtr_primary {
    struct mtr_expr expr_;
    struct mtr_symbol symbol;
};

struct mtr_literal {
    struct mtr_expr expr_;
    struct mtr_token literal;
};

struct mtr_grouping {
    struct mtr_expr expr_;
    struct mtr_expr* expression;
};

struct mtr_binary {
    struct mtr_expr expr_;
    struct mtr_expr* right;
    struct mtr_expr* left;
    struct mtr_symbol operator;
};

struct mtr_call {
    struct mtr_expr expr_;
    struct mtr_symbol symbol;
    struct mtr_expr** argv;
    u8 argc;
};

void mtr_free_expr(struct mtr_expr* node);

#endif
