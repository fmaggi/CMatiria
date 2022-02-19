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
    // struct mtr_expr* body; shoudl the body be kind of a program?
    struct mtr_var_decl* argv; // should variables have their own struct ?
    u8 argc;
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

struct mtr_program {
    struct mtr_decl* declarations;
    size_t size;
    size_t capacity;
};

struct mtr_program mtr_new_program();
void mtr_write_decl(struct mtr_program* program, struct mtr_decl declaration);

#endif
