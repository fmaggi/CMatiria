#ifndef _MTR_PACKAGE_H
#define _MTR_PACKAGE_H

#include "symbol.h"
#include "bytecode.h"
#include "stmt.h"
#include "scope.h"

struct mtr_package {
    struct mtr_scope indices;
    struct mtr_chunk* functions;
    const char* source;
};

struct mtr_package* mtr_new_package(const char* const source, struct mtr_ast* ast);

struct mtr_chunk* mtr_package_get_chunk(struct mtr_package* package, struct mtr_symbol symbol);
struct mtr_chunk* mtr_package_get_chunk_by_name(struct mtr_package* package, const char*);

#endif
