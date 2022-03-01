#ifndef _MTR_STMT_H
#define _MTR_STMT_H

#include "expr.h"
#include "symbol.h"

#include "core/types.h"

enum mtr_stmt_type {
    MTR_STMT_ASSIGNMENT,
    MTR_STMT_FN,
    MTR_STMT_VAR,
    MTR_STMT_IF,
    MTR_STMT_WHILE,
    MTR_STMT_BLOCK
};

struct mtr_stmt {
    enum mtr_stmt_type type;
};

struct mtr_block {
    struct mtr_stmt stmt;
    struct mtr_stmt** statements;
    size_t size;
    size_t capacity;
    u16 var_count;
};

struct mtr_if {
    struct mtr_stmt stmt;
    struct mtr_block* then;
    struct mtr_block* otherwise;
    struct mtr_expr* condition;
};

struct mtr_while {
    struct mtr_stmt stmt;
    struct mtr_block* body;
    struct mtr_expr* condition;
};

struct mtr_variable {
    struct mtr_stmt stmt;
    struct mtr_symbol symbol;
    struct mtr_expr* value;
};

struct mtr_function {
    struct mtr_stmt stmt;
    struct mtr_block* body;
    struct mtr_symbol symbol;
    struct mtr_variable* argv;
    u32 argc;
};

struct mtr_assignment {
    struct mtr_stmt stmt;
    struct mtr_symbol variable;
    struct mtr_expr* expression;
};

struct mtr_ast {
    struct mtr_stmt* head;
};

#endif
