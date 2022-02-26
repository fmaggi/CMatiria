#include "bytecode.h"

#include "core/log.h"

#include <stdlib.h>

struct mtr_chunk mtr_new_chunk() {
    struct mtr_chunk chunk = {
        .bytecode = NULL,
        .capacity = 0,
        .size = 0
    };

    void* temp = malloc(sizeof(u8) * 8);
    if (NULL != temp) {
        chunk.bytecode = temp;
        chunk.capacity = 8;
    } else {
        MTR_LOG_ERROR("Bad allocation.");
    }

    return chunk;
}
void mtr_delete_chunk(struct mtr_chunk* chunk) {
    free(chunk->bytecode);
    chunk->capacity = 0;
    chunk->size = 0;
}

void mtr_write_chunk(struct mtr_chunk* chunk, u8 bytecode) {
    if (chunk->size == chunk->capacity) {
        size_t new_cap = chunk->capacity * 2;
        chunk->bytecode = realloc(chunk->bytecode, new_cap);
        if (NULL == chunk->bytecode) {
            chunk->capacity = 0;
            return;
        }
        chunk->capacity = new_cap;
    }
    chunk->bytecode[chunk->size++] = bytecode;
}
