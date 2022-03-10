#ifndef MTR_STMT_H
#define MTR_STMT_H

#include "expr.h"
#include "symbol.h"

#include "core/types.h"

enum mtr_stmt_type {
    MTR_STMT_ASSIGNMENT,
    MTR_STMT_FN,
    MTR_STMT_NATIVE_FN,
    MTR_STMT_RETURN,
    MTR_STMT_VAR,
    MTR_STMT_IF,
    MTR_STMT_WHILE,
    MTR_STMT_BLOCK,
    MTR_STMT_CALL
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
    struct mtr_stmt* then;
    struct mtr_stmt* otherwise;
    struct mtr_expr* condition;
};

struct mtr_while {
    struct mtr_stmt stmt;
    struct mtr_stmt* body;
    struct mtr_expr* condition;
};

struct mtr_variable {
    struct mtr_stmt stmt;
    struct mtr_symbol symbol;
    struct mtr_expr* value;
};

struct mtr_function_decl {
    struct mtr_stmt stmt;
    struct mtr_block* body;
    struct mtr_symbol symbol;
    struct mtr_variable* argv;
    u8 argc;
};

struct mtr_return {
    struct mtr_stmt stmt;
    struct mtr_function_decl* from;
    struct mtr_expr* expr;
};

struct mtr_assignment {
    struct mtr_stmt stmt;
    struct mtr_symbol variable;
    struct mtr_expr* expression;
};

struct mtr_call_stmt {
    struct mtr_stmt stmt;
    struct mtr_expr* call;
};

struct mtr_ast {
    struct mtr_stmt* head;
};

void mtr_free_stmt(struct mtr_stmt* s);

#endif
