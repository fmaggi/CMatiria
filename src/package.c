#include "package.h"

#include "AST/stmt.h"

#include "core/log.h"
#include "core/utils.h"
#include "runtime/object.h"

#include "debug/dump.h"
#include "validator/scope.h"

#include <stdlib.h>
#include <string.h>

struct mtr_package* mtr_new_package(const char* const source, struct mtr_ast* ast) {
    struct mtr_package* package = malloc(sizeof(struct mtr_package));
    package->source = source;
    package->globals = mtr_new_scope(NULL);

    struct mtr_block* block = (struct mtr_block*) ast->head;
    package->functions = malloc(sizeof(struct mtr_object*) * block->size);

    for (size_t i = 0; i < block->size; ++i) {
        struct mtr_function_decl* f = (struct mtr_function_decl*) block->statements[i];
        MTR_ASSERT(f->stmt.type == MTR_STMT_FN || f->stmt.type == MTR_STMT_NATIVE_FN, "Stmt should be function declaration.");
        MTR_ASSERT(f->symbol.index == i, "Something went wrong!");
        mtr_scope_add(&package->globals, f->symbol);
        package->functions[i] = NULL;
    }

    package->count = block->size;

    return package;
}

void mtr_package_insert_function(struct mtr_package* package, struct mtr_object* object, struct mtr_symbol symbol) {
    const struct mtr_symbol* s = mtr_scope_find(&package->globals, symbol.token);
    if (s == NULL) {
        return;
    }
    package->functions[s->index] = object;
}

void mtr_package_insert_function_by_name(struct mtr_package* package, struct mtr_object* object, const char* name) {
    struct mtr_symbol s;
    s.token.start = name;
    s.token.length = strlen(name);
    mtr_package_insert_function(package, object, s);
}

struct mtr_object* mtr_package_get_function(struct mtr_package* package, struct mtr_symbol symbol) {
    const struct mtr_symbol* s = mtr_scope_find(&package->globals, symbol.token);
    if (s == NULL) {
        return NULL;
    }
    return package->functions[s->index];
}

struct mtr_object* mtr_package_get_function_by_name(struct mtr_package* package, const char* name) {
    struct mtr_symbol s;
    s.token.start = name;
    s.token.length = strlen(name);
    return mtr_package_get_function(package, s);
}

void mtr_delete_package(struct mtr_package* package) {
    for (size_t i = 0; i < package->globals.symbols.size; ++i) {
        mtr_delete_object(package->functions[i]);
    }

    free(package->functions);
    package->functions = NULL;
    mtr_delete_scope(&package->globals);
    free(package);
}
