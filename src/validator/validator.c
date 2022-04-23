#include "validator.h"

#include "AST/symbol.h"
#include "scope.h"

#include "AST/expr.h"
#include "AST/stmt.h"
#include "AST/type.h"

#include "core/report.h"
#include "core/log.h"
#include "debug/dump.h"

#include <stdlib.h>
#include <string.h>

static bool is_literal(struct mtr_expr* expr) {
    return expr->type == MTR_EXPR_ARRAY_LITERAL || expr->type == MTR_EXPR_MAP_LITERAL || expr->type == MTR_EXPR_LITERAL;
}

// static bool is_function(struct mtr_type t) {
//     return t.type == MTR_DATA_FN;
// }

static bool check_assignemnt(struct mtr_type assign_to, struct mtr_type what) {
    if (!mtr_type_match(assign_to, what)) {

        if (assign_to.type == MTR_DATA_UNION) {
            struct mtr_union_type* u = (struct mtr_union_type*) assign_to.obj;
            for (u8 i = 0; i < u->argc; ++i) {
                if (mtr_type_match(u->types[i], what)) {
                    return true;
                }
            }
        }

        return false;
    }
    return true;
}

static void expr_error(struct mtr_expr* expr, const char* message, const char* const source) {
    switch (expr->type)
    {
    case MTR_EXPR_BINARY: {
        struct mtr_binary* b = (struct mtr_binary*) expr;
        mtr_report_error(b->operator.token, message, source);
        break;
    }
    case MTR_EXPR_CALL: {
        struct mtr_call* c = (struct mtr_call*) expr;
        expr_error(c->callable, message, source);
        break;
    }
    case MTR_EXPR_GROUPING: {
        struct mtr_grouping* g = (struct mtr_grouping*) expr;
        expr_error(g->expression, message, source);
        break;
    }
    case MTR_EXPR_LITERAL: {
        struct mtr_literal* l = (struct mtr_literal*) expr;
        mtr_report_error(l->literal, message, source);
        break;
    }
    case MTR_EXPR_PRIMARY: {
        struct mtr_primary* p = (struct mtr_primary*) expr;
        mtr_report_error(p->symbol.token, message, source);
        break;
    }
    case MTR_EXPR_UNARY: {
        struct mtr_unary* u = (struct mtr_unary*) expr;
        mtr_report_error(u->operator.token, message, source);
        break;
    }

    case MTR_EXPR_ACCESS:
    case MTR_EXPR_SUBSCRIPT: {
        struct mtr_access* s = (struct mtr_access*) expr;
        expr_error(s->object, message, source);
        break;
    }

    default:
        break;

    }
}

static struct mtr_type get_operator_type(struct mtr_token op, struct mtr_type lhs, struct mtr_type rhs) {
    struct mtr_type t;

    switch (op.type)
    {
    case MTR_TOKEN_BANG:
    case MTR_TOKEN_OR:
    case MTR_TOKEN_AND:
        t.type = MTR_DATA_BOOL;
        t.obj = NULL;
        break;

    case MTR_TOKEN_PLUS:
    case MTR_TOKEN_MINUS:
    case MTR_TOKEN_STAR:
    case MTR_TOKEN_SLASH:
        t = lhs.type > rhs.type ? lhs : rhs;
        break;
    case MTR_TOKEN_EQUAL:
    case MTR_TOKEN_BANG_EQUAL:
    case MTR_TOKEN_LESS:
    case MTR_TOKEN_LESS_EQUAL:
    case MTR_TOKEN_GREATER:
    case MTR_TOKEN_GREATER_EQUAL:
        t = lhs.type > rhs.type ? lhs : rhs;
        break;
    default:
        t.type = MTR_DATA_INVALID;
        break;
    }
    return t;
}

static struct mtr_expr* try_promoting(struct mtr_expr* expr, struct mtr_type type, struct mtr_type to) {
    switch (type.type) {
    case MTR_DATA_ANY:
    case MTR_DATA_INVALID:
    case MTR_DATA_STRING:
    case MTR_DATA_ARRAY:
    case MTR_DATA_MAP:
    case MTR_DATA_FN:
    case MTR_DATA_FN_COLLECTION:
    case MTR_DATA_VOID:
    case MTR_DATA_USER:
    case MTR_DATA_UNION:
    case MTR_DATA_STRUCT:
        return NULL;
    case MTR_DATA_BOOL:
    case MTR_DATA_INT:
    case MTR_DATA_FLOAT: {
        if (type.type > to.type) {
            return NULL;
        }
        break;
    }

    }

    struct mtr_cast* cast = malloc(sizeof(*cast));
    cast->expr_.type = MTR_EXPR_CAST;
    cast->to = to;
    cast->right = expr;
    return (struct mtr_expr*) cast;
}

static struct mtr_type analyze_expr(struct mtr_expr* expr, struct mtr_scope* scope, const char* const source);

static struct mtr_type analyze_binary(struct mtr_binary* expr, struct mtr_scope* scope, const char* const source) {
    const struct mtr_type l = analyze_expr(expr->left, scope, source);
    const struct mtr_type r = analyze_expr(expr->right, scope, source);

    struct mtr_type t = get_operator_type(expr->operator.token, l, r);

    if (t.type == MTR_DATA_INVALID) {
        mtr_report_error(expr->operator.token, "Invalid operation between objects of different types.", source);
        return invalid_type;
    }

    if (!mtr_type_match(l, r)) {
        // if they dont match, t has type either l or r. Depends which one has higher rank.

        // try and mathc the types. Cast if needed
        if (l.type != t.type) {
            struct mtr_expr* cast = try_promoting(expr->left, l, t);
            if (NULL != cast) {
                expr->left = cast;
            } else {
                mtr_report_error(expr->operator.token, "Invalid operation between objects of different types.", source);
                return invalid_type;
            }
        } else if (r.type != t.type) {
            struct mtr_expr* cast = try_promoting(expr->right, r, t);
            if (NULL != cast) {
                expr->right = cast;
            } else {
                mtr_report_error(expr->operator.token, "Invalid operation between objects of different types.", source);
                return invalid_type;
            }
        }
    }

    expr->operator.type = t;
    return  expr->operator.type;
}

static struct mtr_type analyze_primary(struct mtr_primary* expr, struct mtr_scope* scope, const char* const source) {
    struct mtr_symbol* s = mtr_scope_find(scope, expr->symbol.token);
    if (NULL == s) {
        mtr_report_error(expr->symbol.token, "Undeclared variable.", source);
        return invalid_type;
    }

    expr->symbol.index = s->index;
    expr->symbol.type = s->type;

    return s->type;
}

static struct mtr_type analyze_literal(struct mtr_literal* literal, struct mtr_scope* scope, const char* const source) {
    struct mtr_type t = mtr_get_data_type(literal->literal);
    return t;
}

static struct mtr_type analyze_array_literal(struct mtr_array_literal* array, struct mtr_scope* scope, const char* const source) {
    struct mtr_expr* first = array->expressions[0];
    struct mtr_type array_type = analyze_expr(first, scope, source);

    for (u8 i = 1; i < array->count; ++i) {
        struct mtr_expr* e = array->expressions[i];
        struct mtr_type t = analyze_expr(e, scope, source);
        if (!mtr_type_match(array_type, t)) {
            expr_error(e, "Array literal must contain expressions of the same type", source);
            return invalid_type;
        }
    }

    struct mtr_type type = mtr_new_array_type(mtr_copy_type(array_type));
    return type;
}

static struct mtr_type analyze_map_literal(struct mtr_map_literal* map, struct mtr_scope* scope, const char* const source) {
    struct mtr_map_entry first = map->entries[0];
    struct mtr_type key_type = analyze_expr(first.key, scope, source);
    struct mtr_type val_type = analyze_expr(first.value, scope, source);

    for (u8 i = 1; i < map->count; ++i) {
        struct mtr_map_entry e = map->entries[i];
        struct mtr_type k_t = analyze_expr(e.key, scope, source);
        struct mtr_type v_t = analyze_expr(e.value, scope, source);
        if (!mtr_type_match(key_type, k_t) || !mtr_type_match(val_type, v_t)) {
            expr_error(e.key, "Map literal must contain expressions of the same type", source);
            return invalid_type;
        }
    }

    struct mtr_type type = mtr_new_map_type(key_type, val_type);
    return type;
}

static bool check_params(struct mtr_function_type* f, struct mtr_call* call, struct mtr_scope* scope, const char* const source, bool message) {
    for (u8 i = 0 ; i < call->argc; ++i) {
        struct mtr_expr* a = call->argv[i];
        struct mtr_type from = analyze_expr(a, scope, source);

        struct mtr_type to = f->argv[i];
        bool match = check_assignemnt(to, from);
        if (!match) {
            if (message) {
                expr_error(a, "Wrong type of argument.", source);
            }
            return false;
        }
    }
    return true;
}

// static struct mtr_type curry_call(struct mtr_call* call, struct mtr_type type, struct mtr_scope* scope, const char* const source) {
//     struct mtr_function_type* f = type.obj;
//     bool match = check_params(f, call, scope, source);
//     if (!match) {
//         return invalid_type;
//     }

//     return mtr_new_function_type(f->return_, f->argc - call->argc, f->argv + call->argc);
// }

static struct mtr_type analyze_call(struct mtr_call* call, struct mtr_scope* scope, const char* const source) {
    struct mtr_type type = analyze_expr(call->callable, scope, source);

    if (type.type != MTR_DATA_FN_COLLECTION) {
        expr_error(call->callable, "Expression is not callable.", source);
        return invalid_type;
    }

    struct mtr_function_collection_type* fc = type.obj;
    for (u8 i = 0; i < fc->argc; ++i) {
        struct mtr_function_type* f = fc->functions + i;
        if (f->argc == call->argc && check_params(f, call, scope, source, false)) {
            // This is very hacky and temporary. Just a proof of concept
            struct mtr_primary* p = malloc(sizeof(*p));
            p->expr_.type = MTR_EXPR_PRIMARY;
            p->symbol.index = i;

            struct mtr_access* a = malloc(sizeof(*a));
            a->expr_.type = MTR_EXPR_ACCESS;
            a->object = call->callable;
            a->element = (struct mtr_expr*)p;

            call->callable = (struct mtr_expr*) a;
            return f->return_;
        }
    }
    expr_error(call->callable, "There is no overload with this params.", source);
    return invalid_type;
}

static struct mtr_type analyze_subscript(struct mtr_access* expr, struct mtr_scope* scope, const char* const source) {
    struct mtr_type type = analyze_expr(expr->object, scope, source);
    struct mtr_type index_type = analyze_expr(expr->element, scope, source);

    switch (type.type) {

    case MTR_DATA_ARRAY: {
        if (index_type.type != MTR_DATA_INT) {
            expr_error(expr->element, "Index has to be integral expression.", source);
            return invalid_type;
        }
        break;
    }

    case MTR_DATA_MAP: {
        struct mtr_map_type* m = type.obj;
        if (!mtr_type_match(index_type, m->key)) {
            expr_error(expr->element, "Index doesn't match key type.", source);
            return invalid_type;
        }
        break;
    }

    default:
        expr_error(expr->object, "Expression is not subscriptable.", source);
        return invalid_type;
    }

    struct mtr_type ret = mtr_get_underlying_type(type);
    return ret;
}

static struct mtr_type analyze_unary(struct mtr_unary* expr, struct mtr_scope* scope, const char* const source) {
    const struct mtr_type r = analyze_expr(expr->right, scope, source);
    struct mtr_type dummy = invalid_type;
    expr->operator.type = get_operator_type(expr->operator.token, r, dummy);

    return  expr->operator.type;
}

static struct mtr_type analyze_access(struct mtr_access* expr, struct mtr_scope* scope, const char* const source) {
    const struct mtr_type right_t = analyze_expr(expr->object, scope, source);
    if (right_t.type != MTR_DATA_STRUCT) {
        expr_error(expr->object, "Expression is not accessible.", source);
        return invalid_type;
    }

    if (expr->element->type != MTR_EXPR_PRIMARY) {
        expr_error(expr->element, "Expression cannot be used as access expression.", source);
        return invalid_type;
    }

    struct mtr_primary* p = (struct mtr_primary*) expr->element;
    const struct mtr_struct_type* st = right_t.obj;
    for (u8 i = 0; i < st->argc; ++i) {
        bool match = st->members[i]->token.length == p->symbol.token.length
            && memcmp(st->members[i]->token.start, p->symbol.token.start, p->symbol.token.length) == 0;
        if (match) {
            p->symbol.index = i;
            return st->members[i]->type;
        }
    }
    expr_error(expr->element, "No member.", source);
    return invalid_type;
}

static struct mtr_type analyze_expr(struct mtr_expr* expr, struct mtr_scope* scope, const char* const source) {
    switch (expr->type)
    {
    case MTR_EXPR_BINARY:   return analyze_binary((struct mtr_binary*) expr, scope, source);
    case MTR_EXPR_GROUPING: return analyze_expr(((struct mtr_grouping*) expr)->expression, scope, source);
    case MTR_EXPR_UNARY:    return analyze_unary(((struct mtr_unary*) expr), scope, source);
    case MTR_EXPR_PRIMARY:  return analyze_primary((struct mtr_primary*) expr, scope, source);
    case MTR_EXPR_LITERAL:  return analyze_literal((struct mtr_literal*) expr, scope, source);
    case MTR_EXPR_ARRAY_LITERAL: return analyze_array_literal((struct mtr_array_literal*) expr, scope, source);
    case MTR_EXPR_MAP_LITERAL: return analyze_map_literal((struct mtr_map_literal*) expr, scope, source);
    case MTR_EXPR_CALL:     return analyze_call((struct mtr_call*) expr, scope, source);
    case MTR_EXPR_SUBSCRIPT: return analyze_subscript((struct mtr_access*) expr, scope, source);
    case MTR_EXPR_ACCESS: return analyze_access((struct mtr_access*) expr, scope, source);
    case MTR_EXPR_CAST:     IMPLEMENT return invalid_type;
    }
    MTR_ASSERT(false, "Invalid stmt type.");
    return invalid_type;
}

static bool load_fn(struct mtr_function_decl* stmt, struct mtr_scope* scope, const char* const source) {
    stmt->symbol.type.is_global = true;
    struct mtr_symbol* s = mtr_scope_find(scope, stmt->symbol.token);
    struct mtr_function_type* f = (struct mtr_function_type*) stmt->symbol.type.obj;
    if (NULL != s) {
        struct mtr_function_collection_type* fc = (struct mtr_function_collection_type*) s->type.obj;
        stmt->symbol.index = fc->argc;
        if (mtr_add_function_signature(fc, *f)) {
            free(f);
            stmt->symbol.type = s->type;
            return true;
        }
        mtr_report_error(stmt->symbol.token, "Too many overloads.", source);
        return false;
    }
    struct mtr_function_type types[] = { *f };
    stmt->symbol.type = mtr_new_function_collection_type(types, 1);
    stmt->symbol.index = 0;

    struct mtr_symbol symbol = stmt->symbol;
    symbol.index = scope->current++;
    mtr_symbol_table_insert(&scope->symbols, symbol.token.start, symbol.token.length, symbol);

    free(f);
    return true;
}

static bool load_var(struct mtr_variable* stmt, struct mtr_scope* scope, const char* const source) {
    if (stmt->symbol.type.type == MTR_DATA_ANY) {
        mtr_report_error(stmt->symbol.token, "'Any' expressions are only allowed as parameters to native functions.", source);
        return false;
    }

    const struct mtr_symbol* s = mtr_scope_add(scope, stmt->symbol);
    if (NULL != s) {
        mtr_report_error(stmt->symbol.token, "Redefinition of name.", source);
        mtr_report_message(s->token, "Previuosly defined here.", source);
        return false;
    }
    return true;
}

static struct mtr_stmt* analyze(struct mtr_stmt* stmt, struct mtr_scope* parent, const char* const source);

static struct mtr_stmt* sanitize_stmt(void* stmt, bool condition) {
    if (!condition) {
        mtr_free_stmt(stmt);
        return NULL;
    }
    return stmt;
}

static struct mtr_stmt* analyze_block(struct mtr_block* block, struct mtr_scope* parent, const char* const source) {
    bool all_ok = true;

    struct mtr_scope scope = mtr_new_scope(parent);
    size_t current = scope.current;

    for (size_t i = 0; i < block->size; ++i) {
        struct mtr_stmt* s = block->statements[i];
        struct mtr_stmt* checked = analyze(s, &scope, source);
        block->statements[i] = checked;
        all_ok = checked != NULL && all_ok;
    }

    block->var_count = (u16) (scope.current - current);
    mtr_delete_scope(&scope);
    return sanitize_stmt(block, all_ok);
}

static struct mtr_stmt* analyze_variable(struct mtr_variable* decl, struct mtr_scope* parent, const char* const source) {
    bool expr = true;

    const struct mtr_type value_type = decl->value == NULL ? invalid_type : analyze_expr(decl->value, parent, source);

    if (decl->symbol.type.type == MTR_DATA_INVALID) { // this means it was an implicit var decl
        decl->symbol.type = is_literal(decl->value) ? value_type : mtr_copy_type(value_type);
    }

    if (decl->symbol.type.type == MTR_DATA_USER) {
        // Variable declarations with user types default to the mtr_user_type as we
        // have no way of knowing if it was a struct or a union.
        struct mtr_user_type* type = decl->symbol.type.obj;
        struct mtr_symbol* name = mtr_scope_find(parent, type->name);
        if (NULL == name) {
            mtr_report_error(decl->symbol.token, "Unknown type.", source);
            expr = false;
        }
        // When we check if the type does exist we can now define whether the variable is
        // a struct or a union.

        decl->symbol.type = mtr_copy_type(name->type);
        // free(type); Apparently this causes a double free

        if (!decl->value && decl->symbol.type.type == MTR_DATA_STRUCT) {
            // Create an expression for the constructor
            struct mtr_primary* primary = malloc(sizeof(struct mtr_primary));
            primary->expr_.type = MTR_EXPR_PRIMARY;
            primary->symbol = *name;

            struct mtr_call* constructor = malloc(sizeof(struct mtr_call));
            constructor->expr_.type = MTR_EXPR_CALL;
            constructor->callable = (struct mtr_expr*) primary;
            decl->value = (struct mtr_expr*) constructor;
            goto ret;
        }
    }

    if (decl->value && !check_assignemnt(decl->symbol.type, value_type)) {
        mtr_report_error(decl->symbol.token, "Invalid assignement to variable of different type", source);
        expr = false;
    }

ret:
    decl->symbol.type.assignable = true;
    bool loaded = load_var(decl, parent, source);
    return sanitize_stmt(decl, expr && loaded);
}

static struct mtr_stmt* analyze_fn(struct mtr_function_decl* stmt, struct mtr_scope* parent, const char* const source) {
    bool all_ok = true;

    struct mtr_scope scope = mtr_new_scope(parent);

    for (size_t i = 0; i < stmt->argc; ++i) {
        struct mtr_variable* arg = stmt->argv + i;
        all_ok = analyze_variable(arg, &scope, source) && all_ok;
    }

    struct mtr_stmt* checked = analyze(stmt->body, &scope, source);
    stmt->body = checked;

    all_ok = checked != NULL && all_ok;
    mtr_delete_scope(&scope);

    struct mtr_function_collection_type* t = stmt->symbol.type.obj;
    struct mtr_function_type* type = t->functions + stmt->symbol.index;
    if (type->return_.type != MTR_DATA_VOID) {
        struct mtr_block* body = (struct mtr_block*) stmt->body;
        struct mtr_stmt* last = body->statements[body->size-1];
        if (last->type != MTR_STMT_RETURN) {
            mtr_report_error(stmt->symbol.token, "Non void function doesn't return anything.", source);
            return false;
        }
    }

    return sanitize_stmt(stmt, all_ok);
}

static struct mtr_stmt* analyze_assignment(struct mtr_assignment* stmt, struct mtr_scope* parent, const char* const source) {
    if (stmt->right->type == MTR_EXPR_PRIMARY) {
        struct mtr_primary* p = (struct mtr_primary*) stmt->right;
        struct mtr_symbol* s = mtr_scope_find(parent, p->symbol.token);
        if (NULL == s) {
            struct mtr_variable* v = malloc(sizeof(struct mtr_variable));
            v->stmt.type = MTR_STMT_VAR;
            v->symbol.token = p->symbol.token;
            v->symbol.type = invalid_type;
            v->value = stmt->expression;

            mtr_free_expr((struct mtr_expr*) p);
            free(stmt);
            return analyze_variable(v, parent, source);
        }
    }

    const struct mtr_type right_t = analyze_expr(stmt->right, parent, source);
    if (!right_t.assignable) {
        expr_error(stmt->right, "Expression is not assignable.", source);
        return sanitize_stmt(stmt, false);
    }

    const struct mtr_type expr_t = analyze_expr(stmt->expression, parent, source);

    bool expr_ok = true;
    if (!check_assignemnt(right_t, expr_t)) {
        expr_error(stmt->right, "Invalid assignement to variable of different type", source);
        expr_ok = false;
    }
    return sanitize_stmt(stmt, expr_ok);
}

static struct mtr_stmt* analyze_if(struct mtr_if* stmt, struct mtr_scope* parent, const char* const source) {
    struct mtr_type expr_type = analyze_expr(stmt->condition, parent, source);
    bool condition_ok = expr_type.type == MTR_DATA_FLOAT || expr_type.type == MTR_DATA_INT || expr_type.type == MTR_DATA_BOOL;
    if (!condition_ok) {
        expr_error(stmt->condition, "Expression doesn't return Bool.", source);
    }

    bool then_ok = true;
    {
        struct mtr_stmt* then_checked = analyze(stmt->then, parent, source);
        stmt->then = then_checked;
        then_ok = then_checked != NULL;
    }

    bool e_ok = true;
    if (stmt->otherwise) {
        struct mtr_stmt* e_checked = analyze(stmt->otherwise, parent, source);
        stmt->then = e_checked;
        e_ok = e_checked != NULL;
    }

    return sanitize_stmt(stmt, condition_ok && then_ok && e_ok);
}

static struct mtr_stmt* analyze_while(struct mtr_while* stmt, struct mtr_scope* parent, const char* const source) {
    struct mtr_type expr_type = analyze_expr(stmt->condition, parent, source);
    bool condition_ok = expr_type.type == MTR_DATA_FLOAT || expr_type.type == MTR_DATA_INT || expr_type.type == MTR_DATA_BOOL;
    if (!condition_ok) {
        expr_error(stmt->condition, "Expression doesn't return Bool.", source);
    }

    struct mtr_stmt* body_checked = analyze(stmt->body, parent, source);
    stmt->body = body_checked;
    bool body_ok = body_checked != NULL;

    return sanitize_stmt(stmt, condition_ok && body_ok);
}

static struct mtr_stmt* analyze_return(struct mtr_return* stmt, struct mtr_scope* parent, const char* const source) {
    if (stmt->from->symbol.type.type == MTR_DATA_VOID) {
        if (NULL == stmt->expr) {
            return (struct mtr_stmt*) stmt;
        } else {
            expr_error(stmt->expr, "Void function returns a value.", source);
            return sanitize_stmt(stmt, false);
        }
    }

    MTR_ASSERT(stmt->from->symbol.type.type == MTR_DATA_FN_COLLECTION, "Whaaaaaat!");
    struct mtr_function_collection_type* t = stmt->from->symbol.type.obj;
    struct mtr_type type = t->functions[stmt->from->symbol.index].return_;

    struct mtr_type expr_type = analyze_expr(stmt->expr, parent, source);
    bool ok = mtr_type_match(expr_type, type);
    if (!ok) {
        expr_error(stmt->expr, "Incompatible return type.", source);
        mtr_report_message(stmt->from->symbol.token, "As declared here.", source);
        return sanitize_stmt(stmt, false);
    }
    return (struct mtr_stmt*) stmt;
}

static struct mtr_stmt* analyze_call_stmt(struct mtr_call_stmt* call, struct mtr_scope* scope, const char* const source) {
    struct mtr_type type = analyze_expr(call->call, scope, source);
    return sanitize_stmt(call, type.type != MTR_DATA_INVALID);
}

static struct mtr_stmt* analyze_union(struct mtr_union_decl* u, struct mtr_scope* scope, const char* const source) {
    return (struct mtr_stmt*) u;
}

static struct mtr_stmt* analyze_struct(struct mtr_struct_decl* s, struct mtr_scope* parent, const char* const source) {
    bool all_ok = true;

    struct mtr_scope scope = mtr_new_scope(parent);

    for (size_t i = 0; i < s->argc; ++i) {
        struct mtr_variable* var = s->members[i];
        struct mtr_variable* checked = (struct mtr_variable*) analyze_variable(var, &scope, source);
        s->members[i] = checked;
        all_ok = checked != NULL && all_ok;
    }

    mtr_delete_scope(&scope);

    return sanitize_stmt(s, all_ok);
}

static struct mtr_stmt* analyze(struct mtr_stmt* stmt, struct mtr_scope* scope, const char* const source) {
    switch (stmt->type)
    {
    case MTR_STMT_BLOCK:      return analyze_block((struct mtr_block*) stmt, scope, source);
    case MTR_STMT_ASSIGNMENT: return analyze_assignment((struct mtr_assignment*) stmt, scope, source);
    case MTR_STMT_FN:         return analyze_fn((struct mtr_function_decl*) stmt, scope, source);
    case MTR_STMT_VAR:        return analyze_variable((struct mtr_variable*) stmt, scope, source);
    case MTR_STMT_IF:         return analyze_if((struct mtr_if*) stmt, scope, source);
    case MTR_STMT_WHILE:      return analyze_while((struct mtr_while*) stmt, scope, source);
    case MTR_STMT_RETURN:     return analyze_return((struct mtr_return*) stmt, scope, source);
    case MTR_STMT_CALL:       return analyze_call_stmt((struct mtr_call_stmt*) stmt, scope, source);
    case MTR_STMT_STRUCT:     return analyze_struct((struct mtr_struct_decl*) stmt, scope, source);
    case MTR_STMT_UNION:
    case MTR_STMT_NATIVE_FN:
        return stmt;
    }
    MTR_ASSERT(false, "Invalid stmt type.");
    return false;
}

static struct mtr_stmt* global_analysis(struct mtr_stmt* stmt, struct mtr_scope* scope, const char* const source) {
    switch (stmt->type)
    {
    case MTR_STMT_NATIVE_FN: return stmt;
    case MTR_STMT_FN: return analyze_fn((struct mtr_function_decl*) stmt, scope, source);
    case MTR_STMT_UNION: return analyze_union((struct mtr_union_decl*) stmt, scope, source);
    case MTR_STMT_STRUCT: return analyze_struct((struct mtr_struct_decl*) stmt, scope, source);
    default:
        break;
    }
    MTR_ASSERT(false, "Invalid stmt type.");
    return NULL;
}

static bool load_union(struct mtr_union_decl* u, struct mtr_scope* scope, const char* const source) {
    u->symbol.type.is_global = true;
    const struct mtr_symbol* s = mtr_scope_add(scope, u->symbol);
    if (NULL != s) {
        mtr_report_error(u->symbol.token, "Redefinition of name.", source);
        mtr_report_message(s->token, "Previuosly defined here.", source);
        return false;
    }

    return true;
}

static bool load_struct(struct mtr_struct_decl* st, struct mtr_scope* scope, const char* const source) {
    st->symbol.type.is_global = true;
    const struct mtr_symbol* s = mtr_scope_add(scope, st->symbol);
    if (NULL != s) {
        mtr_report_error(st->symbol.token, "Redefinition of name.", source);
        mtr_report_message(s->token, "Previuosly defined here.", source);
        return false;
    }

    return true;
}

static bool load_global(struct mtr_stmt* stmt, struct mtr_scope* scope, const char* const source) {
    switch (stmt->type)
    {
    case MTR_STMT_NATIVE_FN:
    case MTR_STMT_FN:
        return load_fn((struct mtr_function_decl*) stmt, scope, source);
    case MTR_STMT_UNION:
        return load_union((struct mtr_union_decl*) stmt, scope, source);
    case MTR_STMT_STRUCT:
        return load_struct((struct mtr_struct_decl*) stmt, scope, source);
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

    for (size_t i = 0; i < block->size; ++i) {
        struct mtr_stmt* s = block->statements[i];
        struct mtr_stmt* checked = global_analysis(s, &global, source);
        block->statements[i] = checked;
        all_ok =  checked != NULL && all_ok;
    }

    mtr_delete_scope(&global);
    return all_ok;
}
