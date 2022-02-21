#include "type_analysis.h"

#include "core/log.h"
#include "error.h"

enum var_type {
    ERROR,
    NUMERIC,
};

static enum var_type get_var_type(enum mtr_token_type);

static enum var_type expression(struct mtr_expr* expr);

static enum var_type bianry_expr(struct mtr_binary* expr) {
    enum var_type l = expression(expr->right);
    enum var_type r = expression(expr->left);

    if (l != r) {
        MTR_LOG_ERROR("Type error.");
        return ERROR;
    }

    return l;
}

static enum var_type primary_expr(struct mtr_primary* expr) {
    if (expr->token.type == MTR_TOKEN_IDENTIFIER) {
        IMPLEMENT
    }

    return get_var_type(expr->token.type);
}

static enum var_type unary_expr(struct mtr_unary* expr) {
    return expression(expr->right);
}

static enum var_type grouping_expr(struct mtr_grouping* expr) {
    return expression(expr->expression);
}

static enum var_type expression(struct mtr_expr* expr) {
    switch (expr->type)
    {
    case MTR_EXPR_BINARY:   return bianry_expr((struct mtr_binary*) expr);
    case MTR_EXPR_PRIMARY:  return primary_expr((struct mtr_primary*) expr);
    case MTR_EXPR_UNARY:    return unary_expr((struct mtr_unary*) expr);
    case MTR_EXPR_GROUPING: return grouping_expr((struct mtr_grouping*) expr);
    default:
        break;
    }
}

static enum var_type var_decl(struct mtr_var_decl* decl) {
    enum var_type type = get_var_type(decl->var_type);
    if (decl->value) {
        if (type != expression(decl->value)) {
            MTR_LOG_ERROR("Invalid Type.");
            return ERROR;
        }
    }

    return type;
}

static enum var_type func_decl(struct mtr_fn_decl* decl) {
    return mtr_type_check(decl->body.statements);
}

static enum var_type block(struct mtr_block* block) {
    return mtr_type_check(block->statements);
}

static enum var_type if_stmt(struct mtr_if* stmt) {
    enum var_type e = expression(stmt->condition);
    if (e == ERROR)
        return ERROR;

    enum var_type t = block(&stmt->else_b);
    enum var_type eb = block(&stmt->then);

    if (t == ERROR || eb == ERROR)
        return ERROR;

    return e;
}

static enum var_type while_stmt(struct mtr_while* stmt) {
    IMPLEMENT
    return ERROR;
}

static enum var_type statement(struct mtr_stmt* stmt) {
    switch (stmt->type)
    {
    case MTR_STMT_VAR_DECL:   return var_decl((struct mtr_var_decl*) stmt);
    case MTR_STMT_EXPRESSION: return expression(stmt->expr.expression);
    case MTR_STMT_FUNC:       return func_decl((struct mtr_fn_decl*) stmt);
    case MTR_STMT_BLOCK:      return block((struct mtr_block*) stmt);
    case MTR_STMT_IF:         return if_stmt((struct mtr_if*) stmt);
    case MTR_STMT_WHILE:      return while_stmt((struct mtr_while*) stmt);
    }
    return ERROR;
}

bool mtr_type_check(struct mtr_ast ast) {

    bool ok = true;

    for (size_t i = 0; i < ast.size; ++i) {
        struct mtr_stmt* s = ast.statements + i;
        ok = ok && (bool) statement(s);
    }

    return ok;
}

static enum var_type get_var_type(enum mtr_token_type type) {
    switch (type)
    {
    case MTR_TOKEN_U8:
    case MTR_TOKEN_U16:
    case MTR_TOKEN_U32:
    case MTR_TOKEN_U64:
    case MTR_TOKEN_I8:
    case MTR_TOKEN_I16:
    case MTR_TOKEN_I32:
    case MTR_TOKEN_I64:
    case MTR_TOKEN_F32:
    case MTR_TOKEN_F64:
    case MTR_TOKEN_BOOL:
    case MTR_TOKEN_INT:
    case MTR_TOKEN_FLOAT:
    case MTR_TOKEN_TRUE:
    case MTR_TOKEN_FALSE:
        return NUMERIC;
    default:
        break;
    }

    MTR_LOG_ERROR("Invalid token type");
    return ERROR;
}
