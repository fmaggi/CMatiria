#include "disassemble.h"

#include "bytecode.h"
#include "core/log.h"
#include "runtime/object.h"
#include "runtime/value.h"

u8* mtr_disassemble_instruction(u8* instruction, u32 offset) {
    MTR_PRINT("%04d ", offset);
#define READ(type) *((type*)instruction); instruction += sizeof(type)

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
        MTR_LOG("FLOAT -> %.2f", constant);
        break;
    }

    case MTR_OP_FALSE: {
        MTR_LOG("FALSE");
        break;
    }

    case MTR_OP_TRUE: {
        MTR_LOG("TRUE");
        break;
    }

    case MTR_OP_STRING_LITERAL: {
        const char* s = READ(const char*);
        u32 l = READ(u32);
        MTR_LOG("STR %.*s", l, s);
        break;
    }

    case MTR_OP_ARRAY_LITERAL: {
        u8 count = READ(u8);
        MTR_LOG("ARR (%u)", count);
        break;
    }

    case MTR_OP_MAP_LITERAL: {
        u8 count = READ(u8);
        MTR_LOG("MAP (%u)", count);
        break;
    }

    case MTR_OP_CONSTRUCTOR: {
        u8 count = READ(u8);
        MTR_LOG("CON (%u)", count);
        break;
    }

    case MTR_OP_NIL:
        MTR_LOG("NIL");
        break;

    case MTR_OP_STRING: {
        MTR_LOG("sNEW");
        break;
    }

    case MTR_OP_ARRAY: {
        MTR_LOG("aNEW");
        break;
    }

    case MTR_OP_MAP: {
        MTR_LOG("mNEW");
        break;
    }

    case MTR_OP_OR: {
        MTR_LOG("OR");
        break;
    }

    case MTR_OP_AND: {
        MTR_LOG("AND");
        break;
    }

    case MTR_OP_NOT:
        MTR_LOG("NOT");
        break;

    case MTR_OP_NEGATE_I:
        MTR_LOG("NEG");
        break;

    case MTR_OP_NEGATE_F:
        MTR_LOG("fNEG");
        break;

    case MTR_OP_ADD_I: MTR_LOG("ADD"); break;
    case MTR_OP_SUB_I: MTR_LOG("SUB"); break;
    case MTR_OP_MUL_I: MTR_LOG("MUL"); break;
    case MTR_OP_DIV_I: MTR_LOG("DIV"); break;

    case MTR_OP_ADD_F: MTR_LOG("fADD"); break;
    case MTR_OP_SUB_F: MTR_LOG("fSUB"); break;
    case MTR_OP_MUL_F: MTR_LOG("fMUL"); break;
    case MTR_OP_DIV_F: MTR_LOG("fDIV"); break;

    case MTR_OP_EQUAL_I: MTR_LOG("EQU"); break;
    case MTR_OP_LESS_I: MTR_LOG("LSS"); break;
    case MTR_OP_GREATER_I: MTR_LOG("GTR"); break;

    case MTR_OP_EQUAL_F: MTR_LOG("fEQU"); break;
    case MTR_OP_LESS_F: MTR_LOG("fLSS"); break;
    case MTR_OP_GREATER_F: MTR_LOG("fGTR"); break;

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

    case MTR_OP_GET_O: {
        MTR_LOG("oGET");
        break;
    }

    case MTR_OP_SET_O: {
        MTR_LOG("oSET");
        break;
    }

    case MTR_OP_JMP: {
        i16 to = READ(i16);
        MTR_LOG("JMP %i", to);
        break;
    }

    case MTR_OP_JMP_Z: {
        i16 to = READ(i16);
        MTR_LOG("ZJMP %i", to);
        break;
    }

    case MTR_OP_POP: {
        MTR_LOG("POP");
        break;
    }

    case MTR_OP_POP_V: {
        u16 count = READ(u16);
        MTR_LOG("POPV %u", count);
        break;
    }

    case MTR_OP_CALL: {
        u8 argc = READ(u8);
        MTR_LOG("CALL (%u)", argc);
        break;
    }

    case MTR_OP_FLOAT_CAST: {
        MTR_LOG("fCAST");
        break;
    }
    default:
        MTR_ASSERT(false, "Invalid op code");
    }
    return instruction;
#undef READ
}

void mtr_disassemble(struct mtr_chunk chunk, const char* name) {
    MTR_LOG("====== %s =======", name);
    u8* instruction = chunk.bytecode;
    while (instruction != chunk.bytecode + chunk.size) {
        instruction = mtr_disassemble_instruction(instruction, instruction - chunk.bytecode);
    }
    MTR_LOG("\n");
}

void mtr_dump_stack(mtr_value* stack, mtr_value* top) {
    MTR_PRINT_DEBUG("[");
    while(stack < top) {
        switch (stack->type) {
        case MTR_VAL_INT: MTR_PRINT_DEBUG("%li,", stack->integer); break;
        case MTR_VAL_FLOAT: MTR_PRINT_DEBUG("%f,", stack->floating); break;
        case MTR_VAL_OBJ: MTR_PRINT_DEBUG("%s,", mtr_obj_type_to_str(stack->object)); break;
        }
        stack++;
    }
    MTR_LOG("]");
}

void mtr_dump_chunk(struct mtr_chunk* chunk) {
    u8* ip = chunk->bytecode;
    while (ip < chunk->bytecode + chunk->size) {

        for (int i = 0; i < 8; ++i) {
            u32 offset = ip - chunk->bytecode;
            MTR_LOG("%04u %02x", offset, *ip++);
        }
        MTR_LOG("\n");
    }
}

const char* mtr_obj_type_to_str(struct mtr_object* obj) {
    switch (obj->type) {
    case MTR_OBJ_STRUCT:    return "<struct>";
    case MTR_OBJ_NATIVE_FN: return "<native fn>";
    case MTR_OBJ_FUNCTION:  return "<fn>";
    case MTR_OBJ_ARRAY:     return "<array>";
    case MTR_OBJ_MAP:       return "<map>";
    case MTR_OBJ_STRING:    return "<string>";
    }
}
