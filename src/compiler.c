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

static u64 evaluate_int(struct mtr_token token) {
    u64 s = 0;
    for (u32 i = 0; i < token.length; ++i) {
        s *= 10;
        s += token.start[i] - '0';
    }
    return s;
}

static void evaluate(struct mtr_expr* expr, struct mtr_chunk* chunk);

static void eval_primary(struct mtr_primary* expr, struct mtr_chunk* chunk) {
    mtr_write_chunk(chunk, MTR_OP_CONSTANT);
    u64 value = evaluate_int(expr->token);
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

static void eval_binary(struct mtr_binary* expr, struct mtr_chunk* chunk) {
    evaluate(expr->left, chunk);
    evaluate(expr->right, chunk);

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

static void evaluate(struct mtr_expr* expr, struct mtr_chunk* chunk) {
    switch (expr->type)
    {
    case MTR_EXPR_BINARY:  return eval_binary((struct mtr_binary*) expr, chunk);
    case MTR_EXPR_PRIMARY: return eval_primary((struct mtr_primary*) expr, chunk);
    default:
        break;
    }
}

static void write_var(struct mtr_variable* var, struct mtr_chunk* chunk) {
    evaluate(var->value, chunk);
}

static void write_function(struct mtr_function* fn, struct mtr_chunk* chunk) {
    write_var(&fn->body.statements.statements->variable, chunk);
}

static struct mtr_chunk emit_bytecode(struct mtr_package* package) {
    struct mtr_chunk chunk = mtr_new_chunk();
    write_function(&package->ast.statements->function, &chunk);
    mtr_write_chunk(&chunk, MTR_OP_RETURN);
    return chunk;
}

bool mtr_compile(const char* source) {
    if (NULL == mtr_output_file) {
        mtr_output_file = stdout;
    }

    struct mtr_scanner scanner = mtr_scanner_init(source);
    struct mtr_parser parser = mtr_parser_init(scanner);

    struct mtr_ast ast = mtr_parse(&parser);

    if (parser.had_error){
        return false;
    }

    struct mtr_package package = mtr_new_package(source, ast);
    bool all_ok = mtr_validate(&package);

    if (!all_ok) {
        return false;
    }

    struct mtr_chunk c = emit_bytecode(&package);

    mtr_disassemble(c, "main");

    struct mtr_vm vm;

    mtr_execute(&vm, &c);

    mtr_delete_scope(&package.globals);
    mtr_delete_ast(&ast);
    return true;
}
