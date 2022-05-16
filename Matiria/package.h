#ifndef MTR_PACKAGE_H
#define MTR_PACKAGE_H

#include "AST/AST.h"
#include "runtime/object.h"
#include "validator/symbolTable.h"

struct mtr_package {
    struct mtr_symbol_table symbols;
    struct mtr_object** objects;
    struct mtr_function* main;
    size_t count;
};

void mtr_init_package(struct mtr_package* package);
void mtr_load_package(struct mtr_package* package, struct mtr_ast* ast);
void mtr_delete_package(struct mtr_package* package);

void mtr_package_insert_function(struct mtr_package* package, struct mtr_object* object, struct mtr_symbol symbol);
void mtr_package_insert_native_function(struct mtr_package* package, struct mtr_object* object, const char* name);

struct mtr_object* mtr_package_get_function(struct mtr_package* package, struct mtr_symbol symbol);
struct mtr_object* mtr_package_get_function_by_name(struct mtr_package* package, const char*);

#endif
