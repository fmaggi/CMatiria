#ifndef _MTR_EXPR_H
#define _MTR_EXPR_H

#include "token.h"

enum mtr_expr_type {
    MTR_EXPR_BINARY,
    MTR_EXPR_PRIMARY,
    MTR_EXPR_GROUPING,
    MTR_EXPR_UNARY
};

struct mtr_expr {
    enum mtr_expr_type type;
};

struct mtr_unary {
    struct mtr_expr expr_;
    struct mtr_expr* right;
    enum mtr_token_type operator;
};

struct mtr_primary {
    struct mtr_expr expr_;
    struct mtr_token token;
};

struct mtr_grouping {
    struct mtr_expr expr_;
    struct mtr_expr* expression;
};

struct mtr_binary {
    struct mtr_expr expr_;
    struct mtr_expr* right;
    struct mtr_expr* left;
    enum mtr_token_type operator;
};


void mtr_free_expr(struct mtr_expr* node);

#ifndef NDEBUG

void mtr_print_expr(struct mtr_expr* node);

#endif

#endif
