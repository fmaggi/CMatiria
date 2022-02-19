#ifndef _MTR_STMT_H
#define _MTR_STMT_H

#include "expr.h"

#include "core/types.h"

struct mtr_var_decl {
    struct mtr_token var_type;
    struct mtr_token name;
    struct mtr_expr* value;
};

struct mtr_fn_decl {
    struct mtr_token name;
    struct mtr_token return_type;
    struct mtr_decl* body;

    struct {
        struct mtr_var_decl* argv;
        u8 argc;
    } args;
};

struct mtr_stmt {
    struct mtr_expr* expression;
};

enum mtr_decl_type {
    MTR_DECL_STATEMENT,
    MTR_DECL_FUNC,
    MTR_DECL_VAR_DECL
};

struct mtr_decl {
    union {
        struct mtr_var_decl variable;
        struct mtr_fn_decl function;
        struct mtr_stmt statement;
    };
    enum mtr_decl_type type;
};

struct mtr_ast{
    struct mtr_decl** declarations;
    size_t size;
    size_t capacity;
};

struct mtr_ast mtr_new_ast();
void mtr_write_decl(struct mtr_ast* ast, struct mtr_decl* declaration);
void mtr_delete_ast(struct mtr_ast* ast);

void mtr_print_decl(struct mtr_decl* decl);

#endif
