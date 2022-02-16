#ifndef _MTR_AST_NODES_H
#define _MTR_AST_NODES_H

#include "scanner/token.h"

enum mtr_expr_type {
    MTR_EXPR_BINARY,
    MTR_EXPR_LITERAL,
    MTR_EXPR_GROUPING,
    MTR_EXPR_UNARY
};

struct mtr_expr {
    enum mtr_expr_type type;
};

struct mtr_unary {
    struct mtr_expr expr;
    struct mtr_token operator;
    struct mtr_expr* right;
};

struct mtr_literal {
    struct mtr_expr expr;
    struct mtr_token token;
    union {
        u64 integer;
        f64 floating;
        char* string;
    };
};

struct mtr_grouping {
    struct mtr_expr expr;
    struct mtr_expr* expression;
};

struct mtr_binary {
    struct mtr_expr expr;
    struct mtr_expr* right;
    struct mtr_token operator;
    struct mtr_expr* left;
};

#endif
