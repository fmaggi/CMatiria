#include "validator.h"

#include "core/report.h"
#include "core/log.h"

static const struct mtr_data_type invalid_type = {
    .length = 0,
    .type = MTR_DATA_INVALID,
    .user_struct = NULL
};

static struct mtr_data_type analyze_expr(struct mtr_expr* expr, struct mtr_scope* scope, const char* const source);

static struct mtr_data_type analyze_binary(struct mtr_binary* expr, struct mtr_scope* scope, const char* const source) {
    struct mtr_data_type l = analyze_expr(expr->left, scope, source);
    struct mtr_data_type r = analyze_expr(expr->right, scope, source);

    if (!mtr_data_type_match(l, r)) {
        mtr_report_error(expr->operator, "Invalid operation between objects of different types.", source);
        return invalid_type;
    }

    enum mtr_data_type_e e = mtr_get_data_type(expr->operator.type);

#define CHK(token_type) (expr->operator.type == MTR_TOKEN_ ## token_type)

    if (CHK(STAR) || CHK(SLASH) || CHK(PLUS) || CHK(MINUS)) {
        e = l.type;
    }

#undef CHK

    struct mtr_data_type t = {
        .type = e,
        .length = l.length,
        .user_struct = l.user_struct
    };

    return  t;
}

static struct mtr_data_type analyze_primary(struct mtr_primary* expr, struct mtr_scope* scope, const char* const source) {
    if (expr->token.type == MTR_TOKEN_IDENTIFIER) {
        struct mtr_symbol symbol = {
            .token = expr->token
        };
        struct mtr_symbol* s = mtr_scope_find(scope, symbol);
        if (NULL == s) {
            mtr_report_error(expr->token, "Undeclared variable.", source);
            return invalid_type;
        }
        return s->type;
    }

    struct mtr_data_type t = {
        .length = 0,
        .type = mtr_get_data_type(expr->token.type),
        .user_struct = NULL
    };

    return t;
}

static struct mtr_data_type analyze_expr(struct mtr_expr* expr, struct mtr_scope* scope, const char* const source) {
    switch (expr->type)
    {
    case MTR_EXPR_BINARY:   return analyze_binary((struct mtr_binary*) expr, scope, source);
    case MTR_EXPR_GROUPING: return analyze_expr(((struct mtr_grouping*) expr)->expression, scope, source);
    case MTR_EXPR_UNARY:    return analyze_expr(((struct mtr_unary*) expr)->right, scope, source);
    case MTR_EXPR_PRIMARY:  return analyze_primary((struct mtr_primary*) expr, scope, source);
    default:
        break;
    }
    MTR_ASSERT(false, "Invalid stmt type.");
    return invalid_type;
}

static bool load_fn(struct mtr_fn_decl* stmt, struct mtr_scope* scope, const char* const source) {
    struct mtr_symbol* symbol = mtr_scope_find(scope, stmt->symbol);
    if (NULL != symbol) {
        mtr_report_error(stmt->symbol.token, "Redefinition of name.", source);
        mtr_report_message(symbol->token, "Previuosly defined here.", source);
        return false;
    }

    mtr_symbol_table_insert(&scope->symbols, stmt->symbol.token.start, stmt->symbol.token.length, stmt->symbol);
    return true;
}

static bool load_var(struct mtr_var_decl* stmt, struct mtr_scope* scope, const char* const source) {
    struct mtr_symbol* symbol = mtr_scope_find(scope, stmt->symbol);
    if (NULL != symbol) {
        mtr_report_error(stmt->symbol.token, "Redefinition of name.", source);
        mtr_report_message(symbol->token, "Previuosly defined here.", source);
        return false;
    }

    mtr_symbol_table_insert(&scope->symbols, stmt->symbol.token.start, stmt->symbol.token.length, stmt->symbol);
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

    mtr_delete_scope(&scope);
    return all_ok;
}

static bool analyze_assignment(struct mtr_assignment* stmt, struct mtr_scope* parent, const char* const source) {
    struct mtr_symbol symbol = {
        .token = stmt->variable
    };
    struct mtr_symbol* s = mtr_scope_find(parent, symbol);
    bool var_ok = true;
    if (NULL == s) {
        mtr_report_error(stmt->variable, "Undeclared variable.", source);
        var_ok = false;
    }

    struct mtr_data_type expr = analyze_expr(stmt->expression, parent, source);
    return var_ok && expr.type == s->type.type;
}

static bool analyze_var_decl(struct mtr_var_decl* decl, struct mtr_scope* parent, const char* const source) {
    bool expr = true;
    if (decl->value) {
        struct mtr_data_type type = analyze_expr(decl->value, parent, source);
        expr = mtr_data_type_match(decl->symbol.type, type);
    }

    bool loaded = load_var(decl, parent, source);
    return loaded && expr;
}

static bool analyze_if(struct mtr_if* stmt, struct mtr_scope* parent, const char* const source) {
    bool condition_ok = analyze_expr(stmt->condition, parent, source).type == MTR_DATA_BOOL;
    if (!condition_ok) {
        MTR_LOG_ERROR("Invalid condtion.");
    }

    struct mtr_scope then = mtr_new_scope(parent);
    bool then_ok = analyze_block(&stmt->then, &then, source);
    mtr_delete_scope(&then);

    bool e_ok = true;
    if (stmt->else_b.statements.size > 0) {
        struct mtr_scope e = mtr_new_scope(parent);
        e_ok = analyze_block(&stmt->else_b, &e, source);
        mtr_delete_scope(&e);
    }

    return condition_ok && then_ok && e_ok;
}

static bool analyze_while(struct mtr_while* stmt, struct mtr_scope* parent, const char* const source) {
    bool condition_ok = analyze_expr(stmt->condition, parent, source).type == MTR_DATA_BOOL;
    if (!condition_ok) {
        MTR_LOG_ERROR("Invalid condtion.");
    }

    struct mtr_scope body = mtr_new_scope(parent);
    bool body_ok = analyze_block(&stmt->body, &body, source);
    mtr_delete_scope(&body);

    return condition_ok && body_ok;
}

static bool analyze(struct mtr_stmt* stmt, struct mtr_scope* scope, const char* const source) {
    switch (stmt->type)
    {
    case MTR_STMT_BLOCK:      return analyze_block((struct mtr_block*) stmt, scope, source);
    case MTR_STMT_ASSIGNMENT: return analyze_assignment((struct mtr_assignment*) stmt, scope, source);
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
    case MTR_STMT_FUNC:     return load_fn((struct mtr_fn_decl*) stmt, scope, source);
    default:
        break;
    }
    MTR_ASSERT(false, "Invalid stmt type.");
    return false;
}

bool mtr_validate(struct mtr_package* package) {
    bool all_ok = true;

    for (size_t i = 0; i < package->ast.size; ++i) {
        struct mtr_stmt* s = package->ast.statements + i;
        all_ok = global_analysis(s, &package->globals, package->source) && all_ok;
    }

    return all_ok;
}
