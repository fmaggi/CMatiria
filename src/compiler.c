#include "compiler.h"

#include "bytecode.h"
#include "package.h"
#include "scanner.h"
#include "parser.h"
#include "symbol.h"
#include "validator.h"
#include "vm.h"

#include "core/file.h"
#include "core/log.h"

#include "debug/disassemble.h"

#include <stdlib.h>
#include <string.h>

FILE* mtr_output_file = NULL;

static void write_u64(struct mtr_chunk* chunk, u64 value) {
    // this is definetly dangerous, but fun :). it probably breaks for big endian
    mtr_write_chunk(chunk, (u8) (value >> 0));
    mtr_write_chunk(chunk, (u8) (value >> 8));
    mtr_write_chunk(chunk, (u8) (value >> 16));
    mtr_write_chunk(chunk, (u8) (value >> 24));
    mtr_write_chunk(chunk, (u8) (value >> 32));
    mtr_write_chunk(chunk, (u8) (value >> 40));
    mtr_write_chunk(chunk, (u8) (value >> 48));
    mtr_write_chunk(chunk, (u8) (value >> 56));
}

static u64 evaluate_int(struct mtr_token token) {
    u64 s = 0;
    for (u32 i = 0; i < token.length; ++i) {
        s *= 10;
        s += token.start[i] - '0';
    }
    return s;
}

static void write_expr(struct mtr_chunk* chunk, struct mtr_expr* expr, struct mtr_scope* scope);

static void write_primary(struct mtr_primary* expr, struct mtr_chunk* chunk, struct mtr_scope* scope) {
    if (expr->token.type == MTR_TOKEN_IDENTIFIER) {
        struct mtr_symbol s;
        s.token = expr->token;
        size_t index = mtr_scope_find(scope, s)->index;
        mtr_write_chunk(chunk, MTR_OP_GET);
        write_u64(chunk, index);
    } else {
        mtr_write_chunk(chunk, MTR_OP_CONSTANT);
        u64 value = evaluate_int(expr->token);
        write_u64(chunk, value);
    }
}

static void write_binary(struct mtr_binary* expr, struct mtr_chunk* chunk, struct mtr_scope* scope) {
    write_expr(chunk, expr->left, scope);
    write_expr(chunk, expr->right, scope);

    switch (expr->operator.type)
    {
    case MTR_TOKEN_PLUS:
        mtr_write_chunk(chunk, MTR_OP_PLUS_I);
        break;
    case MTR_TOKEN_MINUS:
        mtr_write_chunk(chunk, MTR_OP_MINUS_I);
        break;
    case MTR_TOKEN_STAR:
        mtr_write_chunk(chunk, MTR_OP_MUL_I);
        break;
    case MTR_TOKEN_SLASH:
        mtr_write_chunk(chunk, MTR_OP_DIV_I);
        break;
    default:
        break;
    }
}

static void write_expr(struct mtr_chunk* chunk, struct mtr_expr* expr, struct mtr_scope* scope) {
    switch (expr->type)
    {
    case MTR_EXPR_BINARY:  return write_binary((struct mtr_binary*) expr, chunk, scope);
    case MTR_EXPR_PRIMARY: return write_primary((struct mtr_primary*) expr, chunk, scope);
    default:
        break;
    }
}

static void write_variable(struct mtr_chunk* chunk, struct mtr_variable* var, struct mtr_scope* scope) {
    write_expr(chunk, var->value, scope);
    mtr_scope_add(scope, var->symbol);
}

static void write(struct mtr_chunk* chunk, struct mtr_stmt* stmt, struct mtr_scope* scope);

static void write_block(struct mtr_chunk* chunk, struct mtr_block* stmt, struct mtr_scope* parent) {
    struct mtr_scope scope = mtr_new_scope(parent);

    for (size_t i = 0; i < stmt->statements.size; ++i) {
        struct mtr_stmt* s = stmt->statements.statements + i;
        write(chunk, s, &scope);
    }

    mtr_delete_scope(&scope);
}

static void write(struct mtr_chunk* chunk, struct mtr_stmt* stmt, struct mtr_scope* scope) {
    switch (stmt->type)
    {
    case MTR_STMT_VAR:   return write_variable(chunk, (struct mtr_variable*) stmt, scope);
    // case MTR_STMT_IF:    return write_if(chunk, (struct mtr_if*) stmt, scope);
    // case MTR_STMT_WHILE: return write_while(chunk, (struct mtr_while*) stmt);
    case MTR_STMT_BLOCK: return write_block(chunk, (struct mtr_block*) stmt, scope);
    // case MTR_STMT_ASSIGNMENT: return write_assignment(chunk, (struct mtr_assignment*) stmt);
    default:
        break;
    }
}

static void write_function(struct mtr_package* package, struct mtr_function* fn) {
    struct mtr_chunk* chunk = mtr_package_get_chunk(package, fn->symbol);
    write_block(chunk, &fn->body, NULL);
    mtr_write_chunk(chunk, MTR_OP_RETURN);
    mtr_disassemble(*chunk, "main");
}

static void write_bytecode(struct mtr_package* package, struct mtr_ast ast) {
    struct mtr_chunk chunk = mtr_new_chunk();

    for (size_t i = 0; i < ast.size; ++i) {
        struct mtr_stmt* s = ast.statements + i;
        MTR_ASSERT(s->type == MTR_STMT_FN, "Stmt type should be function.");
        write_function(package, (struct mtr_function*) s);
    }
}

struct mtr_package* mtr_compile(const char* source) {
    if (NULL == mtr_output_file) {
        mtr_output_file = stdout;
    }

    struct mtr_scanner scanner = mtr_scanner_init(source);
    struct mtr_parser parser = mtr_parser_init(scanner);

    struct mtr_ast ast = mtr_parse(&parser);

    if (parser.had_error){
        return NULL;
    }

    bool all_ok = mtr_validate(&ast, source);

    if (!all_ok) {
        return NULL;
    }

    struct mtr_package* package = mtr_new_package(source, &ast);
    write_bytecode(package, ast);

    mtr_delete_ast(&ast);
    return package;
}
