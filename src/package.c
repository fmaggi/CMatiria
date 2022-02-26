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
    package->functions = malloc(sizeof(struct mtr_chunk) * ast->size);

    for (size_t i = 0; i < ast->size; ++i) {
        struct mtr_stmt* s = ast->statements + i;
        MTR_ASSERT(s->type == MTR_STMT_FN, "Stmt should be function declaration.");
        s->function.symbol.index = i;
        mtr_scope_add(&package->indices, s->function.symbol);
        package->functions[i] = mtr_new_chunk();
    }

    return package;
}

struct mtr_chunk* mtr_package_get_chunk(struct mtr_package* package, struct mtr_symbol symbol) {
    const struct mtr_symbol* s = mtr_scope_find(&package->indices, symbol.token);
    return package->functions + s->index;
}

struct mtr_chunk* mtr_package_get_chunk_by_name(struct mtr_package* package, const char* name) {
    struct mtr_symbol s;
    s.token.start = name;
    s.token.length = strlen(name);
    return mtr_package_get_chunk(package, s);
}
