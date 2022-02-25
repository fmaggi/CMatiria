#include "disassemble.h"

#include "core/log.h"

u8* mtr_disassemble_instruction(u8* instruction) {
    switch (*instruction++)
    {
    case MTR_OP_RETURN:
        MTR_LOG("RETURN");
        break;
    case MTR_OP_CONSTANT: {
            u64 constant = *((u64*)instruction);
            instruction += 8;
            MTR_LOG("CONSTANT -> %lu", constant);
        }
        break;
    case MTR_OP_PLUS_I:
        MTR_LOG("PLUS");
        break;
    case MTR_OP_MUL_I:
        MTR_LOG("MUL");
        break;
    case MTR_OP_MINUS_I:
        MTR_LOG("MINUS");
        break;
    case MTR_OP_DIV_I:
        MTR_LOG("DIV");
        break;
    default:
        break;
    }
    return instruction;
}

void mtr_disassemble(struct mtr_chunk chunk, const char* name) {
    MTR_LOG("====== %s =======", name);
    u8* instruction = chunk.bytecode;
    while (instruction != chunk.bytecode + chunk.size) {
        MTR_PRINT("%04d ", (u32) (instruction - chunk.bytecode));
        instruction = mtr_disassemble_instruction(instruction);
    }
}
