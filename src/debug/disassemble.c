#include "disassemble.h"

#include "core/log.h"

u8* mtr_disassemble_instruction(u8* instruction) {

#define READ(type) *((type*)((void*)instruction)); instruction += sizeof(type)
    switch (*instruction++)
    {
    case MTR_OP_RETURN:
        MTR_LOG("RETURN");
        break;
    case MTR_OP_INT: {
        i64 constant = READ(i64);
        MTR_LOG("INT -> %li", constant);
        break;
    }

    case MTR_OP_FLOAT: {
        f64 constant = READ(f64);
        MTR_LOG("FLOAT -> %f", constant);
        break;
    }

    case MTR_OP_NIL:
        MTR_LOG("NIL");
        break;

    case MTR_OP_PLUS_I:  MTR_LOG("PLUS");  break;
    case MTR_OP_MUL_I:   MTR_LOG("MUL");   break;
    case MTR_OP_MINUS_I: MTR_LOG("MINUS"); break;
    case MTR_OP_DIV_I:   MTR_LOG("DIV");   break;

    case MTR_OP_GET: {
        u16 index = READ(u16);
        MTR_LOG("GET at %u", index);
        break;
    }
    case  MTR_OP_SET: {
        u16 index = READ(u16);
        MTR_LOG("SET at %u", index);
        break;
    }
    default:
        break;
    }
    return instruction;
#undef READ
}

void mtr_disassemble(struct mtr_chunk chunk, const char* name) {
    MTR_LOG("====== %s =======", name);
    u8* instruction = chunk.bytecode;
    while (instruction != chunk.bytecode + chunk.size) {
        MTR_PRINT("%04d ", (u32) (instruction - chunk.bytecode));
        instruction = mtr_disassemble_instruction(instruction);
    }
}

void mtr_dump_stack(mtr_value* stack, mtr_value* top) {
    MTR_LOG("========== STACK ===========");
    while(stack != top) {
        MTR_LOG("[%f]", stack->floating);
        stack++;
    }
    MTR_LOG("============================");
}
