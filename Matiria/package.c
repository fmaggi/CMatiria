#include "package.h"

#include "core/log.h"
#include "core/utils.h"
#include "debug/disassemble.h"

#include "debug/dump.h"

#include <stdlib.h>
#include <string.h>

static bool valid_as_global(struct mtr_stmt* s) {
    return s->type == MTR_STMT_FN
        || s->type == MTR_STMT_NATIVE_FN
        || s->type == MTR_STMT_STRUCT
        || s->type == MTR_STMT_UNION;
}

static struct mtr_symbol get_symbol(struct mtr_stmt* s) {
    switch (s->type) {
    case MTR_STMT_STRUCT: {
        struct mtr_struct_decl* sd = (struct mtr_struct_decl*) s;
        return sd->symbol;
    }
    case MTR_STMT_UNION: {
        struct mtr_union_decl* ud = (struct mtr_union_decl*) s;
        return ud->symbol;
    }
    case MTR_STMT_FN:
    case MTR_STMT_NATIVE_FN: {
        struct mtr_function_decl* fd = (struct mtr_function_decl*) s;
        return fd->symbol;
    }
    default:
        MTR_ASSERT(false, "Invalid stmt");
    }
    return (struct mtr_symbol){ .index = 0};
}

static bool is_main(struct mtr_symbol symbol) {
    return symbol.token.length == strlen("main") && memcmp(symbol.token.start, "main", strlen("main")) == 0;
}

void mtr_init_package(struct mtr_package* package) {
    package->count = 0;
    package->objects = NULL;
    package->main = NULL;
    mtr_init_symbol_table(&package->symbols);
}

void mtr_load_package(struct mtr_package* package, struct mtr_ast* ast) {
    (void) valid_as_global;
    struct mtr_block* block = (struct mtr_block*) ast->head;
    package->objects = malloc(sizeof(struct mtr_object*) * block->size);
    package->main = NULL;

    for (size_t i = 0; i < block->size; ++i) {
        MTR_ASSERT(valid_as_global(block->statements[i]), "Statement is not valid as global statement!");
        struct mtr_symbol s = get_symbol(block->statements[i]);
        MTR_ASSERT(s.index == i, "Wrong index");
        mtr_symbol_table_insert(&package->symbols, s.token.start, s.token.length, s);
        package->objects[i] = NULL;
    }

    package->count = block->size;
}

void mtr_package_insert_function(struct mtr_package* package, struct mtr_object* object, struct mtr_symbol symbol) {
    const struct mtr_symbol* s = mtr_symbol_table_get(&package->symbols, symbol.token.start, symbol.token.length);
    if (s == NULL) {
        MTR_LOG_WARN("Name '%.*s' not found!", symbol.token.length, symbol.token.start);
        return;
    }

    if (is_main(*s)) {
        package->main = (struct mtr_function*) object;
    }

    package->objects[s->index] = object;
}

void mtr_package_insert_native_function(struct mtr_package* package, struct mtr_object* object, const char* name) {
    struct mtr_token t;
    t.start = name;
    t.length = strlen(name);
    const struct mtr_symbol* s = mtr_symbol_table_get(&package->symbols, t.start, t.length);
    if (s == NULL) {
        MTR_LOG_WARN("Name '%.*s' not found!", t.length, t.start);
        return;
    }

    if (is_main(*s)) {
        package->main = (struct mtr_function*) object;
    }

    package->objects[s->index] = object;
}

struct mtr_object* mtr_package_get_function(struct mtr_package* package, struct mtr_symbol symbol) {
    const struct mtr_symbol* s = mtr_symbol_table_get(&package->symbols, symbol.token.start, symbol.token.length);
    if (s == NULL) {
        return NULL;
    }
    return package->objects[s->index];
}

struct mtr_object* mtr_package_get_function_by_name(struct mtr_package* package, const char* name) {
    struct mtr_symbol s;
    s.token.start = name;
    s.token.length = strlen(name);
    return mtr_package_get_function(package, s);
}

void mtr_delete_package(struct mtr_package* package) {
    for (size_t i = 0; i < package->symbols.size; ++i) {
        if (!package->objects[i]) continue;
        mtr_delete_object(package->objects[i]);
    }

    free(package->objects);
    package->objects = NULL;
    mtr_delete_symbol_table(&package->symbols);
}
