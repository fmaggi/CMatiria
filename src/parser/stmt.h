#ifndef _MTR_STMT_H
#define _MTR_STMT_H

#include "expr.h"

#include "core/types.h"

struct mtr_stmt;

struct mtr_declaration {

};

struct mtr_stmt {
    struct mtr_expr* expression;
};

#endif
