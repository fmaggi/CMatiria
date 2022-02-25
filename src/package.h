#ifndef _MTR_PACKAGE_H
#define _MTR_PACKAGE_H

#include "scope.h"
#include "stmt.h"
#include "bytecode.h"

struct mtr_package {
    struct mtr_ast ast;
    struct mtr_scope globals;

    struct mtr_chunk* chunks;
    const char* source;
};

struct mtr_package mtr_new_package(const char* const source, struct mtr_ast ast);

#endif
