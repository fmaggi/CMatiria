#ifndef MTR_BYTECODE_H
#define MTR_BYTECODE_H

#include "core/types.h"

enum mtr_op_code {
    MTR_OP_INT,
    MTR_OP_FLOAT,

    MTR_OP_FALSE,
    MTR_OP_TRUE,

    MTR_OP_STRING_LITERAL,
    MTR_OP_ARRAY_LITERAL,
    MTR_OP_MAP_LITERAL,
    MTR_OP_CONSTRUCTOR,

    MTR_OP_NIL,

    MTR_OP_STRING,
    MTR_OP_ARRAY,
    MTR_OP_MAP,
    MTR_OP_STRUCT,

    MTR_OP_OR,
    MTR_OP_AND,

    MTR_OP_NOT,

    MTR_OP_NEGATE_I,
    MTR_OP_NEGATE_F,

    MTR_OP_ADD_I,
    MTR_OP_SUB_I,
    MTR_OP_MUL_I,
    MTR_OP_DIV_I,

    MTR_OP_ADD_F,
    MTR_OP_SUB_F,
    MTR_OP_MUL_F,
    MTR_OP_DIV_F,

    MTR_OP_LESS_I,
    MTR_OP_GREATER_I,
    MTR_OP_EQUAL_I,

    MTR_OP_LESS_F,
    MTR_OP_GREATER_F,
    MTR_OP_EQUAL_F,

    MTR_OP_GET,
    MTR_OP_SET,

    MTR_OP_GET_O,
    MTR_OP_SET_O,

    MTR_OP_JMP,
    MTR_OP_JMP_Z,

    MTR_OP_POP,
    MTR_OP_POP_V,

    MTR_OP_CALL,

    MTR_OP_INT_CAST,
    MTR_OP_FLOAT_CAST,

    MTR_OP_RETURN
};

struct mtr_chunk {
    u8* bytecode;
    size_t size;
    size_t capacity;
};

struct mtr_chunk mtr_new_chunk(void);
void mtr_delete_chunk(struct mtr_chunk* chunk);

void mtr_write_chunk(struct mtr_chunk* chunk, u8 bytecode);

#endif
