#ifndef MTR_PACKAGE_H
#define MTR_PACKAGE_H

#include "bytecode.h"
#include "AST/stmt.h"
#include "runtime/object.h"
#include "validator/scope.h"
#include "runtime/value.h"

struct mtr_package {
    struct mtr_scope globals;
    struct mtr_object** functions;
    const char* source;
};

struct mtr_package* mtr_new_package(const char* const source, struct mtr_ast* ast);
void mtr_delete_package(struct mtr_package* package);

void mtr_package_insert_function(struct mtr_package* package, struct mtr_object* object, struct mtr_symbol symbol);
void mtr_package_insert_function_by_name(struct mtr_package* package, struct mtr_object* object, const char* name);

struct mtr_object* mtr_package_get_function(struct mtr_package* package, struct mtr_symbol symbol);
struct mtr_object* mtr_package_get_function_by_name(struct mtr_package* package, const char*);

#endif
