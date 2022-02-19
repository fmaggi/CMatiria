#ifndef _MTR_STMT_H
#define _MTR_STMT_H

#include "expr.h"

#include "core/types.h"

enum mtr_stmt_type {
    MTR_STMT_EXPRESSION,
    MTR_STMT_FUNC,
    MTR_STMT_VAR_DECL,
    MTR_STMT_BLOCK
};

struct mtr_ast {
    struct mtr_stmt** statements;
    size_t size;
    size_t capacity;
};

struct mtr_block {
    struct mtr_ast statements;
};

struct mtr_var_decl {
    struct mtr_token name;
    struct mtr_expr* value;
    enum mtr_token_type var_type;
};

struct mtr_fn_decl {
    struct mtr_token name;

    struct {
        struct mtr_var_decl* argv;
        u8 argc;
    } args;

    struct mtr_stmt* body;
    enum mtr_token_type return_type;
};

struct mtr_expr_stmt {
    struct mtr_expr* expression;
};

struct mtr_stmt {
    union {
        struct mtr_var_decl variable;
        struct mtr_fn_decl function;
        struct mtr_expr_stmt statement;
        struct mtr_block block;
    };
    enum mtr_stmt_type type;
};

struct mtr_ast mtr_new_ast();
void mtr_write_stmt(struct mtr_ast* ast, struct mtr_stmt* declaration);
void mtr_delete_ast(struct mtr_ast* ast);

void mtr_print_stmt(struct mtr_stmt* decl);

#endif
