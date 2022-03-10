#ifndef MTR_DISASSEMBLE_H
#define MTR_DISASSEMBLE_H

#include "runtime/value.h"
#include "bytecode.h"

void mtr_disassemble(struct mtr_chunk chunk, const char* name);
u8* mtr_disassemble_instruction(u8* instruction, u32 offset);

void mtr_dump_stack(mtr_value* stack, mtr_value* top);
void mtr_dump_chunk(struct mtr_chunk* chunk);

#endif
