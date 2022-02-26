#ifndef _MTR_BYTECODE_H
#define _MTR_BYTECODE_H

#include "core/types.h"

enum mtr_op_code {
    MTR_OP_CONSTANT,
    MTR_OP_NIL,
    MTR_OP_PLUS_I,
    MTR_OP_MINUS_I,
    MTR_OP_MUL_I,
    MTR_OP_DIV_I,
    MTR_OP_GET,
    MTR_OP_SET,
    MTR_OP_RETURN,
};

struct mtr_chunk {
    u8* bytecode;
    size_t size;
    size_t capacity;
};

struct mtr_chunk mtr_new_chunk();
void mtr_delete_chunk(struct mtr_chunk* chunk);

void mtr_write_chunk(struct mtr_chunk* chunk, u8 bytecode);

#endif
