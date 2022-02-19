#ifndef _MTR_EXPR_H
#define _MTR_EXPR_H

#include "scanner/token.h"

enum mtr_expr_type {
    MTR_EXPR_BINARY,
    MTR_EXPR_PRIMARY,
    MTR_EXPR_GROUPING,
    MTR_EXPR_UNARY
};

struct mtr_expr;

struct mtr_unary {
    enum mtr_token_type operator;
    struct mtr_expr* right;
};

struct mtr_primary {
    struct mtr_token token;
};

struct mtr_grouping {
    struct mtr_expr* expression;
};

struct mtr_binary {
    enum mtr_token_type operator;
    struct mtr_expr* right;
    struct mtr_expr* left;
};


// I use a union here because Im thinking about allocating exprs in a linear array in the future.
// It uses more memory and the logic is a bit messier but it could be faster
struct mtr_expr {

    union {
        struct mtr_binary binary;
        struct mtr_unary unary;
        struct mtr_primary primary;
        struct mtr_grouping grouping;
    };

    enum mtr_expr_type type;
};

void mtr_print_expr(struct mtr_expr* node);

void mtr_free_expr(struct mtr_expr* node);

#endif
