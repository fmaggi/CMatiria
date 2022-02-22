#include "semantic_analysis.h"

#include "core/report.h"
#include "core/log.h"

static bool analyze_expr(struct mtr_expr* stmt, struct mtr_scope* scope, const char* const source) {
    IMPLEMENT
    return true;
}

static bool load_fn(struct mtr_fn_decl* stmt, struct mtr_scope* scope, const char* const source) {
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

static bool load_var(struct mtr_var_decl* stmt, struct mtr_scope* scope, const char* const source) {
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

static bool analyze(struct mtr_stmt* stmt, struct mtr_scope* parent, const char* const source);

static bool analyze_block(struct mtr_block* block, struct mtr_scope* parent, const char* const source) {
    bool all_ok = true;

    for (size_t i = 0; i < block->statements.size; ++i) {
        struct mtr_stmt* s = block->statements.statements + i;
        all_ok = analyze(s, parent, source) && all_ok;
    }

    return all_ok;
}

static bool analyze_fn(struct mtr_fn_decl* stmt, struct mtr_scope* parent, const char* const source) {
    bool all_ok = true;

    struct mtr_scope scope = mtr_new_scope(parent);

    for (size_t i = 0; i < stmt->argc; ++i) {
        struct mtr_var_decl* arg = stmt->argv + i;
        all_ok = load_var(arg, &scope, source) && all_ok;
    }

    all_ok = analyze_block(&stmt->body, &scope, source) && all_ok;

    mtr_delete_symbol_table(&scope.symbols);
    return all_ok;
}

static bool analyze_expr_stmt(struct mtr_expr_stmt* stmt, struct mtr_scope* parent, const char* const source) {
    return analyze_expr(stmt->expression, parent, source);
}

static bool analyze_var_decl(struct mtr_var_decl* decl, struct mtr_scope* parent, const char* const source) {
    bool loaded = load_var(decl, parent, source);
    bool expr = true;
    if (decl->value) {
        expr = analyze_expr(decl->value, parent, source);
    }

    return loaded && expr;
}

static bool analyze_if(struct mtr_if* stmt, struct mtr_scope* parent, const char* const source) {
    MTR_PROFILE_FUNC();
    bool condition_ok = analyze_expr(stmt->condition, parent, source);

    struct mtr_scope then = mtr_new_scope(parent);
    bool then_ok = analyze_block(&stmt->then, &then, source);

    mtr_delete_symbol_table(&then.symbols);


    bool e_ok = true;
    if (stmt->else_b.statements.size > 0) {
        struct mtr_scope e = mtr_new_scope(parent);
        e_ok = analyze_block(&stmt->else_b, &e, source);
        mtr_delete_symbol_table(&e.symbols);
    }

    return condition_ok && then_ok && e_ok;
}

static bool analyze_while(struct mtr_while* stmt, struct mtr_scope* parent, const char* const source) {
    bool condition_ok = analyze_expr(stmt->condition, parent, source);

    struct mtr_scope body = mtr_new_scope(parent);
    bool body_ok = analyze_block(&stmt->body, &body, source);

    mtr_delete_symbol_table(&body.symbols);

    return condition_ok && body_ok;
}

static bool analyze(struct mtr_stmt* stmt, struct mtr_scope* scope, const char* const source) {
    switch (stmt->type)
    {
    case MTR_STMT_BLOCK:      return analyze_block((struct mtr_block*) stmt, scope, source);
    case MTR_STMT_EXPRESSION: return analyze_expr_stmt((struct mtr_expr_stmt*) stmt, scope, source);
    case MTR_STMT_FUNC:       return analyze_fn((struct mtr_fn_decl*) stmt, scope, source);
    case MTR_STMT_VAR_DECL:   return analyze_var_decl((struct mtr_var_decl*) stmt, scope, source);
    case MTR_STMT_IF:         return analyze_if((struct mtr_if*) stmt, scope, source);
    case MTR_STMT_WHILE:      return analyze_while((struct mtr_while*) stmt, scope, source);
    default:
        break;
    }
    MTR_ASSERT(false, "Invalid stmt type.");
    return false;
}

static bool global_analysis(struct mtr_stmt* stmt, struct mtr_scope* scope, const char* const source) {
    switch (stmt->type)
    {
    case MTR_STMT_VAR_DECL: return true; // global vars already added
    case MTR_STMT_FUNC: return analyze_fn((struct mtr_fn_decl*) stmt, scope, source);
    default:
        break;
    }
    MTR_ASSERT(false, "Invalid stmt type.");
    return false;
}

static bool load_global(struct mtr_stmt* stmt, struct mtr_scope* scope, const char* const source) {
    switch (stmt->type)
    {
    case MTR_STMT_VAR_DECL: return load_var((struct mtr_var_decl*) stmt, scope, source);
    case MTR_STMT_FUNC:     return load_fn((struct mtr_fn_decl*) stmt, scope, source);
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
        all_ok = load_global(global, &package->globals, package->source) && all_ok;
    }

    for (size_t i = 0; i < package->ast.size; ++i) {
        struct mtr_stmt* s = package->ast.statements + i;
        all_ok = global_analysis(s, &package->globals, package->source) && all_ok;
    }

    return all_ok;
}
