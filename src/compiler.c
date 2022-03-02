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
#include "debug/dump.h"

#include <stdlib.h>
#include <string.h>

struct compiler {
    struct mtr_chunk* chunk;
    u16 count;
};

static u64 evaluate_int(struct mtr_token token) {
    u64 s = 0;
    for (u32 i = 0; i < token.length; ++i) {
        s *= 10;
        s += token.start[i] - '0';
    }
    return s;
}

static f64 evaluate_float(struct mtr_token token) {
    f64 s = 0;
    const char* c = token.start;
    while (*c != '.') {
        s *= 10;
        s += *c - '0';
        c++;
    }

    c++;

    u32 i = 1;
    while (c != token.start + token.length) {
        f64 x = (f64)(*c - '0') / (f64)(i * 10);
        s += x;
        c++;
        i++;
    }

    return s;
}

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

static void write_u32(struct mtr_chunk* chunk, u32 value) {
    mtr_write_chunk(chunk, (u8) (value >> 0));
    mtr_write_chunk(chunk, (u8) (value >> 8));
    mtr_write_chunk(chunk, (u8) (value >> 16));
    mtr_write_chunk(chunk, (u8) (value >> 24));
}

static void write_u16(struct mtr_chunk* chunk, u16 value) {
    mtr_write_chunk(chunk, (u8) (value >> 0));
    mtr_write_chunk(chunk, (u8) (value >> 8));
}

#define AS(type, value) *((type*)&value)

// returns the location of where to jump relative to the chunk
static u16 write_jump(struct mtr_chunk* chunk, u8 instruction) {
    mtr_write_chunk(chunk, instruction);
    write_u16(chunk, (u16) 0xFFFFu);
    return chunk->size - 2;
}

// patch the jump address after a block of bytecode
static void patch_jump(struct mtr_chunk* chunk, i16 offset) {
    i16 where = chunk->size - offset - 2;
    i16* to_patch = (i16*)(chunk->bytecode + offset);
    *to_patch = where;
}

static void write_loop(struct mtr_chunk* chunk, u16 offset) {
    mtr_write_chunk(chunk, MTR_OP_JMP);
    i16 where = offset - chunk->size - 3;
    write_u16(chunk, AS(u16, where));
}

static void pop_v(struct compiler* compiler) {
    mtr_write_chunk(compiler->chunk, MTR_OP_POP_V);
    write_u16(compiler->chunk, compiler->count);
}

static void write_expr(struct compiler* compiler, struct mtr_expr* expr);

static void write_primary(struct compiler* compiler, struct mtr_primary* expr) {
    mtr_write_chunk(compiler->chunk, MTR_OP_GET);
    write_u16(compiler->chunk, expr->symbol.index);
    compiler->count = 0;
}

static void write_literal(struct compiler* compiler, struct mtr_literal* expr) {
    switch (expr->literal.type)
    {
    case MTR_TOKEN_INT_LITERAL: {
        mtr_write_chunk(compiler->chunk, MTR_OP_INT);
        u64 value = evaluate_int(expr->literal);
        write_u64(compiler->chunk, value);
        break;
    }

    case MTR_TOKEN_FLOAT_LITERAL: {
        mtr_write_chunk(compiler->chunk, MTR_OP_FLOAT);
        f64 value = evaluate_float(expr->literal);
        write_u64(compiler->chunk, AS(u64, value));
        break;
    }

    case MTR_TOKEN_TRUE: {
        mtr_write_chunk(compiler->chunk, MTR_OP_TRUE);
        break;
    }

    case MTR_TOKEN_FALSE: {
        mtr_write_chunk(compiler->chunk, MTR_OP_FALSE);
        break;
    }
    default:
        MTR_LOG_WARN("Invalid type");
        break;
    }
}

static void write_binary(struct compiler* compiler, struct mtr_binary* expr) {
    write_expr(compiler, expr->left);
    write_expr(compiler, expr->right);

    switch (expr->operator.token.type)
    {
    case MTR_TOKEN_PLUS:
        if (expr->operator.type.type == MTR_DATA_INT) {
            mtr_write_chunk(compiler->chunk, MTR_OP_ADD_I);
        } else {
            mtr_write_chunk(compiler->chunk, MTR_OP_ADD_F);
        }
        break;
    case MTR_TOKEN_MINUS:
        if (expr->operator.type.type == MTR_DATA_INT) {
            mtr_write_chunk(compiler->chunk, MTR_OP_SUB_I);
        } else {
            mtr_write_chunk(compiler->chunk, MTR_OP_SUB_F);
        }
        break;
    case MTR_TOKEN_STAR:
        if (expr->operator.type.type == MTR_DATA_INT) {
            mtr_write_chunk(compiler->chunk, MTR_OP_MUL_I);
        } else {
            mtr_write_chunk(compiler->chunk, MTR_OP_MUL_F);
        }
        break;
    case MTR_TOKEN_SLASH:
        if (expr->operator.type.type == MTR_DATA_INT) {
            mtr_write_chunk(compiler->chunk, MTR_OP_DIV_I);
        } else {
            mtr_write_chunk(compiler->chunk, MTR_OP_DIV_F);
        }
        break;
    default:
        break;
    }
}

static void write_unary(struct compiler* compiler, struct mtr_unary* unary) {
    write_expr(compiler, unary->right);

    switch (unary->operator.token.type)
    {
    case MTR_TOKEN_BANG:
        mtr_write_chunk(compiler->chunk, MTR_OP_NOT);
        break;
    case MTR_TOKEN_MINUS:
        if (unary->operator.type.type == MTR_DATA_INT) {
            mtr_write_chunk(compiler->chunk, MTR_OP_NEGATE_I);
        } else {
            mtr_write_chunk(compiler->chunk, MTR_OP_NEGATE_F);
        }
        break;
    default:
        break;
    }
}

static void write_call(struct compiler* compiler, struct mtr_call* call) {
    for (u8 i = 0; i < call->argc; ++i) {
        struct mtr_expr* expr = call->argv[i];
        write_expr(compiler, expr);
    }

    mtr_write_chunk(compiler->chunk, MTR_OP_CALL);
    write_u16(compiler->chunk, call->symbol.index);
    mtr_write_chunk(compiler->chunk, call->argc);
}

static void write_expr(struct compiler* compiler, struct mtr_expr* expr) {
    switch (expr->type)
    {
    case MTR_EXPR_BINARY:  return write_binary(compiler, (struct mtr_binary*) expr);
    case MTR_EXPR_PRIMARY: return write_primary(compiler, (struct mtr_primary*) expr);
    case MTR_EXPR_LITERAL: return write_literal(compiler, (struct mtr_literal*) expr);
    case MTR_EXPR_UNARY:   return write_unary(compiler, (struct mtr_unary*) expr);
    case MTR_EXPR_GROUPING: return write_expr(compiler, ((struct mtr_grouping*) expr)->expression);
    case MTR_EXPR_CALL: return write_call(compiler, (struct mtr_call*) expr);
    default:
        break;
    }
}

static void write(struct compiler* compiler, struct mtr_stmt* stmt);

static void write_variable(struct compiler* compiler, struct mtr_variable* var) {
    if (var->value) {
        write_expr(compiler, var->value);
    } else {
        mtr_write_chunk(compiler->chunk, MTR_OP_NIL);
    }
    compiler->count++;
}

static void write_block(struct compiler* compiler, struct mtr_block* stmt) {
    struct compiler block_c;
    block_c.chunk = compiler->chunk;
    block_c.count = 0;

    for (size_t i = 0; i < stmt->size; ++i) {
        struct mtr_stmt* s = stmt->statements[i];
        write(&block_c, s);
    }
    pop_v(&block_c);
}

static void write_if(struct compiler* compiler, struct mtr_if* stmt) {
    write_expr(compiler, stmt->condition);
    u16 offset = write_jump(compiler->chunk, MTR_OP_JMP_Z);
    mtr_write_chunk(compiler->chunk, MTR_OP_POP);

    write_block(compiler, stmt->then);

    u16 otherwise = write_jump(compiler->chunk, MTR_OP_JMP);

    patch_jump(compiler->chunk, offset);
    mtr_write_chunk(compiler->chunk, MTR_OP_POP);

    if (stmt->otherwise) {
        write_block(compiler, stmt->otherwise);
    }
    patch_jump(compiler->chunk, otherwise);
}

static void write_while(struct compiler* compiler, struct mtr_while* stmt) {
    write_expr(compiler, stmt->condition);
    u16 offset = write_jump(compiler->chunk, MTR_OP_JMP_Z);
    mtr_write_chunk(compiler->chunk, MTR_OP_POP);

    write_block(compiler, stmt->body);

    write_loop(compiler->chunk, offset);

    patch_jump(compiler->chunk, offset);
    mtr_write_chunk(compiler->chunk, MTR_OP_POP);
}

static void write_assignment(struct compiler* compiler, struct mtr_assignment* stmt) {
    write_expr(compiler, stmt->expression);
    mtr_write_chunk(compiler->chunk, MTR_OP_SET);
    write_u16(compiler->chunk, stmt->variable.index);
}

static void write_return(struct compiler* compiler, struct mtr_return* stmt) {
    if (stmt->expr) {
        write_expr(compiler, stmt->expr);
    } else {
        mtr_write_chunk(compiler->chunk, MTR_OP_NIL);
    }

    mtr_write_chunk(compiler->chunk, MTR_OP_RETURN);
}

static void write(struct compiler* compiler, struct mtr_stmt* stmt) {
    switch (stmt->type)
    {
    case MTR_STMT_VAR:   return write_variable(compiler, (struct mtr_variable*) stmt);
    case MTR_STMT_IF:    return write_if(compiler, (struct mtr_if*) stmt);
    case MTR_STMT_WHILE: return write_while(compiler, (struct mtr_while*) stmt);
    case MTR_STMT_BLOCK: return write_block(compiler, (struct mtr_block*) stmt);
    case MTR_STMT_ASSIGNMENT: return write_assignment(compiler, (struct mtr_assignment*) stmt);
    case MTR_STMT_RETURN: return write_return(compiler, (struct mtr_return*) stmt);
    default:
        break;
    }
}

static void write_function(struct compiler* compiler, struct mtr_function* fn) {
    struct mtr_block* b = fn->body;
    for (size_t i = 0; i < b->size; ++i) {
        struct mtr_stmt* s = b->statements[i];
        write(compiler, s);
    }
}


// as every function has its own chunk we could probably paralellize this pretty easily
static void write_bytecode(struct mtr_stmt* stmt, struct mtr_package* package) {
    struct compiler compiler;
    switch (stmt->type)
    {
    case MTR_STMT_FN: {
        struct mtr_function* fn = (struct mtr_function*) stmt;
        compiler.chunk = mtr_package_get_chunk(package, fn->symbol);
        write_function(&compiler, fn);
        break;
    }
    default:
        break;
    }
}

#undef AS

struct mtr_package* mtr_compile(const char* source) {
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

    struct mtr_block* block = (struct mtr_block*) ast.head;
    for (size_t i = 0; i < block->size; ++i) {
        struct mtr_stmt* s = block->statements[i];
        write_bytecode(s, package);
    }

    mtr_delete_ast(&ast);
    return package;
}
