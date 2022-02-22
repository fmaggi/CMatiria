#include "semantic_analysis.h"

#include "error.h"

#include "core/log.h"

static bool load_global_var(struct mtr_var_decl* stmt, struct mtr_scope* scope, const char* const source) {
    struct mtr_symbol s;
    s.token = stmt->name;
    s.type.type = mtr_get_data_type(stmt->var_type);

    struct mtr_symbol* symbol = mtr_scope_find(scope, stmt->name.start, stmt->name.length);
    if (NULL != symbol) {
        mtr_report_error(stmt->name, "Redefinition of name.", source);
        mtr_report_message(symbol->token, "Previuosly defined here.", source);
        return false;
    }

    mtr_symbol_table_insert(&scope->symbols, stmt->name.start, stmt->name.length, s);
    return true;
}

static bool load_global_fn(struct mtr_fn_decl* stmt, struct mtr_scope* scope, const char* const source) {
    struct mtr_symbol s;
    s.token = stmt->name;
    s.type.type = mtr_get_data_type(stmt->return_type);

    struct mtr_symbol* symbol = mtr_scope_find(scope, stmt->name.start, stmt->name.length);
    if (NULL != symbol) {
        mtr_report_error(stmt->name, "Redefinition of name.", source);
        mtr_report_message(symbol->token, "Previuosly defined here.", source);
        return false;
    }

    mtr_symbol_table_insert(&scope->symbols, stmt->name.start, stmt->name.length, s);
    return true;
}

static bool load_global(struct mtr_stmt* stmt, struct mtr_scope* scope, const char* const source) {
    switch (stmt->type)
    {
    case MTR_STMT_VAR_DECL: return load_global_var((struct mtr_var_decl*) stmt, scope, source);
    case MTR_STMT_FUNC:     return load_global_fn((struct mtr_fn_decl*) stmt, scope, source);
    default:
        break;
    }
    MTR_ASSERT(false, "Invalid stmt type.");
    return false;
}

bool mtr_perform_semantic_analysis(struct mtr_package* package) {

    bool all_ok = true;

    for (size_t i = 0; i < package->ast.size; ++i) {
        struct mtr_stmt* global = package->ast.statements + i;
        all_ok = all_ok && load_global(global, &package->global_scope, package->source);
    }

    return true;
}
