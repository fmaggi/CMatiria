#ifndef _MTR_STMT_H
#define _MTR_STMT_H

#include "expr.h"
#include "symbol.h"

#include "core/types.h"

enum mtr_stmt_type {
    MTR_STMT_ASSIGNMENT,
    MTR_STMT_FUNC,
    MTR_STMT_VAR_DECL,
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
    struct mtr_block else_b;
    struct mtr_expr* condition;
};

struct mtr_while {
    struct mtr_block body;
    struct mtr_expr* condition;
};

struct mtr_var_decl {
    struct mtr_symbol symbol;
    struct mtr_expr* value;
};

struct mtr_fn_decl {
    struct mtr_block body;
    struct mtr_symbol symbol;
    struct mtr_var_decl* argv;
    u32 argc;
};

struct mtr_assignment {
    struct mtr_token variable;
    struct mtr_expr* expression;
};

// This wastes to much memory. Every stmt is 72 bytes. Dont know if it is worth it
// it could lead to less indirection and thus more performance but...
struct mtr_stmt {
    union {
        struct mtr_assignment assignment;
        struct mtr_fn_decl function;
        struct mtr_var_decl variable;
        struct mtr_if if_s;
        struct mtr_while while_s;
        struct mtr_block block;
    };
    enum mtr_stmt_type type;
};

struct mtr_ast mtr_new_ast();
void mtr_write_stmt(struct mtr_ast* ast, struct mtr_stmt declaration);
void mtr_delete_ast(struct mtr_ast* ast);

#ifndef NDEBUG

void mtr_print_stmt(struct mtr_stmt* decl);

#endif

#endif
