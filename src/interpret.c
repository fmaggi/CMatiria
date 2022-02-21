#include "interpret.h"

#include "object.h"

#include "core/log.h"

typedef void (*interpret_fn)(struct mtr_stmt*);

static void interpret_block(struct mtr_stmt*);
static void interpret_expression(struct mtr_stmt*);
static void interpret_fn_decl(struct mtr_stmt*);
static void interpret_if(struct mtr_stmt*);
static void interpret_var_decl(struct mtr_stmt*);
static void interpret_while(struct mtr_stmt*);

static const interpret_fn interpret_funcs[] = {
    [MTR_STMT_BLOCK] = interpret_block,
    [MTR_STMT_EXPRESSION] = interpret_expression,
    [MTR_STMT_FUNC] = interpret_fn_decl,
    [MTR_STMT_IF] = interpret_if,
    [MTR_STMT_VAR_DECL] = interpret_var_decl,
    [MTR_STMT_WHILE] = interpret_while
};

static void interpret(struct mtr_stmt* stmt) {
    interpret_fn fn = interpret_funcs[stmt->type];
    fn(stmt);
}

void mtr_interpret(struct mtr_ast ast) {
    for (size_t i = 0; i < ast.size; ++i) {
        struct mtr_stmt* s = ast.statements + i;
        interpret(s);
    }
}

static void interpret_block(struct mtr_stmt* stmt) {
    struct mtr_block* b = &stmt->block;
    mtr_interpret(b->statements);
}

static void interpret_fn_decl(struct mtr_stmt* stmt) {
    IMPLEMENT
}
static void interpret_if(struct mtr_stmt* stmt) {
    IMPLEMENT
}
static void interpret_var_decl(struct mtr_stmt* stmt) {
    IMPLEMENT
}
static void interpret_while(struct mtr_stmt* stmt) {
    IMPLEMENT
}

static struct mtr_object evaluate(struct mtr_expr*);

static void interpret_expression(struct mtr_stmt* stmt) {
    struct mtr_expr_stmt* e = &stmt->expr;
    struct mtr_expr* expr = e->expression;
    struct mtr_object o = evaluate(expr);
    MTR_LOG_DEBUG("%lu", o.as.integer);
}

typedef struct mtr_object (*evaluate_fn)(struct mtr_expr*);

static struct mtr_object eval_binary(struct mtr_expr*);
static struct mtr_object eval_grouping(struct mtr_expr*);
static struct mtr_object eval_unary(struct mtr_expr*);
static struct mtr_object eval_primary(struct mtr_expr*);

static const evaluate_fn eval_funcs[] = {
    [MTR_EXPR_BINARY] = eval_binary,
    [MTR_EXPR_GROUPING] = eval_grouping,
    [MTR_EXPR_UNARY] = eval_unary,
    [MTR_EXPR_PRIMARY] = eval_primary
};

static struct mtr_object evaluate(struct mtr_expr* expr) {
    evaluate_fn fn = eval_funcs[expr->type];
    return fn(expr);
}

static struct mtr_object eval_binary(struct mtr_expr* expr) {
    struct mtr_binary* b = (struct mtr_binary*) expr;

    struct mtr_object l = evaluate(b->left);
    struct mtr_object r = evaluate(b->right);

    MTR_ASSERT(l.type == r.type, "Invalid types.");

    switch (b->operator)
    {
    case MTR_TOKEN_PLUS:
        l.as.integer += r.as.integer;
        break;
    case MTR_TOKEN_MINUS:
        l.as.integer -= r.as.integer;
        break;
    case MTR_TOKEN_STAR:
        l.as.integer *= r.as.integer;
        break;
    case MTR_TOKEN_SLASH:
        l.as.integer /= r.as.integer;
        break;
    default:
        MTR_LOG_ERROR("Invalid binary operator %s", mtr_token_type_to_str(b->operator));
    }

    return l;
}

static struct mtr_object eval_grouping(struct mtr_expr* expr) {
    struct mtr_grouping* g = (struct mtr_grouping*) expr;
    return evaluate(g->expression);
}

static struct mtr_object eval_unary(struct mtr_expr* expr) {
    IMPLEMENT;
    struct mtr_object o;
    return o;
}

static u64 evaluate_int(struct mtr_token token) {
    u64 s = 0;
    for (u32 i = 0; i < token.length; ++i) {
        s *= 10;
        s += token.start[i] - '0';
    }
    return s;
}

static struct mtr_object eval_primary(struct mtr_expr* expr) {
    struct mtr_primary* p = (struct mtr_primary*) expr;
    struct mtr_object obj;
    obj.type = MTR_OBJ_INT;
    obj.as.integer = evaluate_int(p->token);
    return obj;
}
