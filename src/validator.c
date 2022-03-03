#include "validator.h"

#include "scope.h"

#include "core/report.h"
#include "core/log.h"
#include "debug/dump.h"

static const struct mtr_data_type invalid_type = {
    .length = 0,
    .type = MTR_DATA_INVALID,
    .user_struct = NULL
};

static struct mtr_data_type get_operator_type(struct mtr_token op, struct mtr_data_type lhs, struct mtr_data_type rhs) {
    struct mtr_data_type t;

    switch (op.type)
    {
    case MTR_TOKEN_BANG:
    case MTR_TOKEN_EQUAL:
    case MTR_TOKEN_BANG_EQUAL:
    case MTR_TOKEN_LESS:
    case MTR_TOKEN_LESS_EQUAL:
    case MTR_TOKEN_GREATER:
    case MTR_TOKEN_GREATER_EQUAL:
    case MTR_TOKEN_OR:
    case MTR_TOKEN_AND:
        t.type = MTR_DATA_BOOL;
        t.length = 0;
        t.user_struct = NULL;
        break;
    case MTR_TOKEN_PLUS:
    case MTR_TOKEN_MINUS:
    case MTR_TOKEN_STAR:
    case MTR_TOKEN_SLASH:
        t = lhs;
        break;
    default:
        t.type = MTR_DATA_INVALID;
        break;
    }
    return t;
}

static const struct mtr_data_type analyze_expr(struct mtr_expr* expr, struct mtr_scope* scope, const char* const source);

static struct mtr_data_type analyze_binary(struct mtr_binary* expr, struct mtr_scope* scope, const char* const source) {
    const struct mtr_data_type l = analyze_expr(expr->left, scope, source);
    const struct mtr_data_type r = analyze_expr(expr->right, scope, source);

    if (!mtr_data_type_match(l, r)) {
        mtr_report_error(expr->operator.token, "Invalid operation between objects of different types.", source);
        return invalid_type;
    }

    expr->operator.type = get_operator_type(expr->operator.token, l, r);
    return  expr->operator.type;
}

static const struct mtr_data_type analyze_primary(struct mtr_primary* expr, struct mtr_scope* scope, const char* const source) {
    struct mtr_symbol_entry* e = mtr_scope_find(scope, expr->symbol.token);
    if (NULL == e) {
        mtr_report_error(expr->symbol.token, "Undeclared variable.", source);
        return invalid_type;
    }

    struct mtr_symbol s = e->symbol;
    expr->symbol.index = s.index;
    expr->symbol.type = s.type;
    return s.type;
}

static const struct mtr_data_type analyze_literal(struct mtr_literal* literal, struct mtr_scope* scope, const char* const source) {
    struct mtr_data_type t;
    t.length = 0;
    t.user_struct = NULL;
    t.type = mtr_get_data_type(literal->literal.type);
    return t;
}

static const struct mtr_data_type analyze_call(struct mtr_call* call, struct mtr_scope* scope, const char* const source) {
    struct mtr_symbol_entry* e = mtr_scope_find(scope, call->symbol.token);
    if (NULL == e) {
        mtr_report_error(call->symbol.token, "Call to undefined function.", source);
        return invalid_type;
    }

    struct mtr_symbol s = e->symbol;
    struct mtr_function* f = (struct mtr_function*) e->parent;
    if (f->argc == call->argc) {
        for (u8 i =0 ; i < call->argc; ++i) {
            struct mtr_expr* a = call->argv[i];
            struct mtr_data_type ta = analyze_expr(a, scope, source);

            struct mtr_variable p = f->argv[i];
            if (!mtr_data_type_match(ta, p.symbol.type)) {
                MTR_LOG_ERROR("Wrong type of argument passed");
            }
        }


    } else if (f->argc > call->argc) {
        mtr_report_error(call->symbol.token, "Expected more arguments.", source);
    } else {
        mtr_report_error(call->symbol.token, "Too many arguments.", source);
    }

    call->symbol.index = s.index;
    call->symbol.type = s.type;
    return s.type;
}

static const struct mtr_data_type analyze_unary(struct mtr_unary* expr, struct mtr_scope* scope, const char* const source) {
    const struct mtr_data_type r = analyze_expr(expr->right, scope, source);
    struct mtr_data_type dummy;
    expr->operator.type = get_operator_type(expr->operator.token, r, dummy);

    return  expr->operator.type;
}

static const struct mtr_data_type analyze_expr(struct mtr_expr* expr, struct mtr_scope* scope, const char* const source) {
    switch (expr->type)
    {
    case MTR_EXPR_BINARY:   return analyze_binary((struct mtr_binary*) expr, scope, source);
    case MTR_EXPR_GROUPING: return analyze_expr(((struct mtr_grouping*) expr)->expression, scope, source);
    case MTR_EXPR_UNARY:    return analyze_unary(((struct mtr_unary*) expr), scope, source);
    case MTR_EXPR_PRIMARY:  return analyze_primary((struct mtr_primary*) expr, scope, source);
    case MTR_EXPR_LITERAL:  return analyze_literal((struct mtr_literal*) expr, scope, source);
    case MTR_EXPR_CALL:     return analyze_call((struct mtr_call*) expr, scope, source);
    default:
        break;
    }
    MTR_ASSERT(false, "Invalid stmt type.");
    return invalid_type;
}

static bool load_fn(struct mtr_function* stmt, struct mtr_scope* scope, const char* const source) {
    const struct mtr_symbol_entry* e = mtr_scope_find(scope, stmt->symbol.token);
    if (NULL != e) {
        mtr_report_error(stmt->symbol.token, "Redefinition of name.", source);
        mtr_report_message(e->symbol.token, "Previuosly defined here.", source);
        return false;
    }

    stmt->symbol.index = scope->current++;
    mtr_scope_add(scope, stmt->symbol, (struct mtr_stmt*) stmt);
    return true;
}

static bool load_var(struct mtr_variable* stmt, struct mtr_scope* scope, const char* const source) {
    const struct mtr_symbol_entry* e = mtr_scope_find(scope, stmt->symbol.token);
    if (NULL != e) {
        mtr_report_error(stmt->symbol.token, "Redefinition of name.", source);
        mtr_report_message(e->symbol.token, "Previuosly defined here.", source);
        return false;
    }

    stmt->symbol.index = scope->current++;
    mtr_scope_add(scope, stmt->symbol, (struct mtr_stmt*) stmt);
    return true;
}

static bool analyze(struct mtr_stmt* stmt, struct mtr_scope* parent, const char* const source);

static bool analyze_block(struct mtr_block* block, struct mtr_scope* parent, const char* const source) {
    bool all_ok = true;

    struct mtr_scope scope = mtr_new_scope(parent);
    size_t current = scope.current;

    for (size_t i = 0; i < block->size; ++i) {
        struct mtr_stmt* s = block->statements[i];
        bool s_ok = analyze(s, &scope, source);
        all_ok = s_ok && all_ok;
    }

    block->var_count = (u16) (scope.current - current);
    mtr_delete_scope(&scope);
    return all_ok;
}

static bool analyze_fn(struct mtr_function* stmt, struct mtr_scope* parent, const char* const source) {
    bool all_ok = true;

    struct mtr_scope scope = mtr_new_scope(parent);

    for (size_t i = 0; i < stmt->argc; ++i) {
        struct mtr_variable* arg = stmt->argv + i;
        all_ok = load_var(arg, &scope, source) && all_ok;
    }

    all_ok = analyze_block(stmt->body, &scope, source) && all_ok;
    mtr_delete_scope(&scope);

    return all_ok;
}

static bool analyze_assignment(struct mtr_assignment* stmt, struct mtr_scope* parent, const char* const source) {
    const struct mtr_symbol_entry* e = mtr_scope_find(parent, stmt->variable.token);
    bool var_ok = true;
    if (NULL == e) {
        mtr_report_error(stmt->variable.token, "Undeclared variable.", source);
        var_ok = false;
    }
    struct mtr_symbol s = e->symbol;
    stmt->variable.index = s.index;
    stmt->variable.type = s.type;
    const struct mtr_data_type expr = analyze_expr(stmt->expression, parent, source);
    bool expr_ok = mtr_data_type_match(expr, s.type);
    if (!expr_ok) {
        mtr_report_error(stmt->variable.token, "Invalid assignement to variable of different type", source);
    }

    return var_ok && expr_ok;
}

static bool analyze_variable(struct mtr_variable* decl, struct mtr_scope* parent, const char* const source) {
    bool expr = true;
    if (decl->value) {
        const struct mtr_data_type type = analyze_expr(decl->value, parent, source);
        expr = mtr_data_type_match(decl->symbol.type, type);
        if (!expr) {
            mtr_report_error(decl->symbol.token, "Invalid expression to variable of different type", source);
        }
    }

    bool loaded = load_var(decl, parent, source);
    return loaded && expr;
}

static bool analyze_if(struct mtr_if* stmt, struct mtr_scope* parent, const char* const source) {
    bool condition_ok = analyze_expr(stmt->condition, parent, source).type == MTR_DATA_BOOL;
    if (!condition_ok) {
        MTR_LOG_ERROR("Invalid condtion.");
    }

    bool then_ok = analyze_block(stmt->then, parent, source);

    bool e_ok = true;
    if (stmt->otherwise) {
        e_ok = analyze_block(stmt->otherwise, parent, source);
    }

    return condition_ok && then_ok && e_ok;
}

static bool analyze_while(struct mtr_while* stmt, struct mtr_scope* parent, const char* const source) {
    bool condition_ok = analyze_expr(stmt->condition, parent, source).type == MTR_DATA_BOOL;
    if (!condition_ok) {
        MTR_LOG_ERROR("Invalid condtion.");
    }

    bool body_ok = analyze_block(stmt->body, parent, source);

    return condition_ok && body_ok;
}

static bool analyze_return(struct mtr_return* stmt, struct mtr_scope* parent, const char* const source) {
    bool ok = mtr_data_type_match(analyze_expr(stmt->expr, parent, source), stmt->from.type);
    if (!ok) {
        MTR_LOG_ERROR("Wrong return type.");
    }
    return ok;
}

static bool analyze(struct mtr_stmt* stmt, struct mtr_scope* scope, const char* const source) {
    switch (stmt->type)
    {
    case MTR_STMT_BLOCK:      return analyze_block((struct mtr_block*) stmt, scope, source);
    case MTR_STMT_ASSIGNMENT: return analyze_assignment((struct mtr_assignment*) stmt, scope, source);
    case MTR_STMT_FN:         return analyze_fn((struct mtr_function*) stmt, scope, source);
    case MTR_STMT_VAR:        return analyze_variable((struct mtr_variable*) stmt, scope, source);
    case MTR_STMT_IF:         return analyze_if((struct mtr_if*) stmt, scope, source);
    case MTR_STMT_WHILE:      return analyze_while((struct mtr_while*) stmt, scope, source);
    case MTR_STMT_RETURN:     return analyze_return((struct mtr_return*) stmt, scope, source);
    default:
        break;
    }
    MTR_ASSERT(false, "Invalid stmt type.");
    return false;
}

static bool global_analysis(struct mtr_stmt* stmt, struct mtr_scope* scope, const char* const source) {
    switch (stmt->type)
    {
    case MTR_STMT_FN: return analyze_fn((struct mtr_function*) stmt, scope, source);
    default:
        break;
    }
    MTR_ASSERT(false, "Invalid stmt type.");
    return false;
}

static bool load_global(struct mtr_stmt* stmt, struct mtr_scope* scope, const char* const source) {
    switch (stmt->type)
    {
    case MTR_STMT_FN:     return load_fn((struct mtr_function*) stmt, scope, source);
    default:
        break;
    }
    MTR_ASSERT(false, "Invalid stmt type.");
    return false;
}

bool mtr_validate(struct mtr_ast* ast, const char* const source) {
    bool all_ok = true;

    struct mtr_scope global = mtr_new_scope(NULL);

    struct mtr_block* block = (struct mtr_block*) ast->head;

    for (size_t i = 0; i < block->size; ++i) {
        struct mtr_stmt* s = block->statements[i];
        all_ok = load_global(s, &global, source);
    }

    global.current = 0;

    for (size_t i = 0; i < block->size; ++i) {
        struct mtr_stmt* s = block->statements[i];
        all_ok = global_analysis(s, &global, source) && all_ok;
    }

    mtr_delete_scope(&global);

    return all_ok;
}
