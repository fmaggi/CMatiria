#include "package.h"

#include "AST/stmt.h"

#include "AST/symbol.h"
#include "core/log.h"
#include "core/utils.h"
#include "runtime/object.h"

#include "debug/dump.h"
#include "validator/scope.h"

#include <stdlib.h>
#include <string.h>

static bool valid_as_global(struct mtr_stmt* s) {
    return s->type == MTR_STMT_FN
        || s->type == MTR_STMT_NATIVE_FN
        || s->type == MTR_STMT_STRUCT;
}

static struct mtr_symbol get_symbol(struct mtr_stmt* s) {
    switch (s->type) {
    case MTR_STMT_STRUCT: {
        struct mtr_struct_decl* sd = (struct mtr_struct_decl*) s;
        return sd->symbol;
    }
    case MTR_STMT_FN:
    case MTR_STMT_NATIVE_FN: {
        struct mtr_function_decl* fd = (struct mtr_function_decl*) s;
        return fd->symbol;
    }
    default:
        exit(1);
    }
}

struct mtr_package* mtr_new_package(const char* const source, struct mtr_ast* ast) {
    struct mtr_package* package = malloc(sizeof(struct mtr_package));
    package->source = source;
    package->globals = mtr_new_scope(NULL);

    struct mtr_block* block = (struct mtr_block*) ast->head;
    package->functions = malloc(sizeof(struct mtr_object*) * block->size);

    for (size_t i = 0; i < block->size; ++i) {
        MTR_ASSERT(valid_as_global(block->statements[i]), "Statement is not valid as global statement!");
        struct mtr_symbol s = get_symbol(block->statements[i]);
        mtr_scope_add(&package->globals, s);
        package->functions[i] = NULL;
    }

    package->count = block->size;

    return package;
}

void mtr_package_insert_function(struct mtr_package* package, struct mtr_object* object, struct mtr_symbol symbol) {
    const struct mtr_symbol* s = mtr_scope_find(&package->globals, symbol.token);
    if (s == NULL) {
        MTR_LOG_WARN("Name not found!");
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
