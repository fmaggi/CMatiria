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

struct mtr_ast {
    struct mtr_stmt* statements;
    size_t size;
    size_t capacity;
};

struct mtr_block {
    struct mtr_ast statements;
};

struct mtr_if {
    struct mtr_block then;
    struct mtr_block otherwise;
    struct mtr_expr* condition;
};

struct mtr_while {
    struct mtr_block body;
    struct mtr_expr* condition;
};

struct mtr_variable {
    struct mtr_symbol symbol;
    struct mtr_expr* value;
};

struct mtr_function {
    struct mtr_block body;
    struct mtr_symbol symbol;
    struct mtr_variable* argv;
    u32 argc;
};

struct mtr_assignment {
    struct mtr_symbol variable;
    struct mtr_expr* expression;
};

// This wastes to much memory. Every stmt is 72 bytes. Dont know if it is worth it
// it could lead to less indirection and thus more performance but...
struct mtr_stmt {
    union {
        struct mtr_assignment assignment;
        struct mtr_function function;
        struct mtr_variable variable;
        struct mtr_if if_s;
        struct mtr_while while_s;
        struct mtr_block block;
    };
    enum mtr_stmt_type type;
};

struct mtr_ast mtr_new_ast();
void mtr_write_stmt(struct mtr_ast* ast, struct mtr_stmt declaration);
void mtr_delete_ast(struct mtr_ast* ast);

#endif
