#include "compiler.h"

#include "bytecode.h"
#include "package.h"
#include "scanner.h"
#include "parser.h"
#include "symbol.h"
#include "validator.h"

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

static u64 evaluate(struct mtr_expr* expr) {
    struct mtr_primary* p = (struct mtr_primary*) expr;
    return evaluate_int(p->token);
}

static void write_var(struct mtr_var_decl* var, struct mtr_chunk* chunk) {
    mtr_write_chunk(chunk, MTR_OP_CONSTANT);
    u64 value = evaluate(var->value);
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

static void write_function(struct mtr_fn_decl* fn, struct mtr_chunk* chunk) {
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

    mtr_delete_scope(&package.globals);
    mtr_delete_ast(&ast);
    return true;
}
