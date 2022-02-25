#ifndef _MTR_DISASSEMBLE_H
#define _MTR_DISASSEMBLE_H

#include "bytecode.h"

void mtr_disassemble(struct mtr_chunk chunk, const char* name);
u8* mtr_disassemble_instruction(u8* instruction);

#endif
