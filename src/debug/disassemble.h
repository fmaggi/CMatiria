#ifndef _MTR_DISASSEMBLE_H
#define _MTR_DISASSEMBLE_H

#include "value.h"
#include "bytecode.h"

void mtr_disassemble(struct mtr_chunk chunk, const char* name);
u8* mtr_disassemble_instruction(u8* instruction);

void mtr_dump_stack(mtr_value* stack, mtr_value* top);

#endif
