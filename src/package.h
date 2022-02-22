#ifndef _MTR_PACKAGE_H
#define _MTR_PACKAGE_H

#include "symbol.h"

struct mtr_package {
    struct mtr_ast ast;
    struct mtr_scope global_scope;
    const char* const source;
};

struct mtr_package mtr_new_package(const char* const source, struct mtr_ast ast);

#endif
