#include "package.h"

#include "stmt.h"

#include "core/log.h"
#include "core/utils.h"

#include <stdlib.h>
#include <string.h>

struct mtr_package* mtr_new_package(const char* const source, struct mtr_ast* ast) {
    struct mtr_package* package = malloc(sizeof(struct mtr_package));
    package->source = source;
    package->indices = mtr_new_scope(NULL);

    struct mtr_block* block = (struct mtr_block*) ast->head;
    package->functions = malloc(sizeof(struct mtr_chunk) * block->size);

    for (size_t i = 0; i < block->size; ++i) {
        struct mtr_function* f = (struct mtr_function*) block->statements[i];
        MTR_ASSERT(f->stmt.type == MTR_STMT_FN, "Stmt should be function declaration.");
        f->symbol.index = i;
        mtr_scope_add(&package->indices, f->symbol, (struct mtr_stmt*) f);
        package->functions[i] = mtr_new_chunk();
    }

    return package;
}

struct mtr_chunk* mtr_package_get_chunk(struct mtr_package* package, struct mtr_symbol symbol) {
    const struct mtr_symbol_entry* s = mtr_scope_find(&package->indices, symbol.token);
    return package->functions + s->symbol.index;
}

struct mtr_chunk* mtr_package_get_chunk_by_name(struct mtr_package* package, const char* name) {
    struct mtr_symbol s;
    s.token.start = name;
    s.token.length = strlen(name);
    return mtr_package_get_chunk(package, s);
}

void mtr_delete_package(struct mtr_package* package) {
    for (size_t i = 0; i < package->indices.symbols.size; ++i) {
        mtr_delete_chunk(package->functions + i);
    }
    free(package->functions);
    package->functions = NULL;
    mtr_delete_scope(&package->indices);
    free(package);
}
