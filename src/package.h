#ifndef MTR_PACKAGE_H
#define MTR_PACKAGE_H

#include "bytecode.h"
#include "AST/stmt.h"
#include "validator/scope.h"

struct mtr_package {
    struct mtr_scope globals;
    struct mtr_chunk* functions;
    const char* source;
};

struct mtr_package* mtr_new_package(const char* const source, struct mtr_ast* ast);
void mtr_delete_package(struct mtr_package* package);

struct mtr_chunk* mtr_package_get_chunk(struct mtr_package* package, struct mtr_symbol symbol);
struct mtr_chunk* mtr_package_get_chunk_by_name(struct mtr_package* package, const char*);

#endif
