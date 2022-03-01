#include "vm.h"

#include "core/log.h"
#include "debug/disassemble.h"

void dump_value(mtr_value value, enum mtr_data_type_e type) {
    if (type == MTR_DATA_FLOAT) {
        MTR_LOG_DEBUG("%.3f", value.floating);
    } else {
        MTR_LOG_DEBUG("%li", value.integer);
    }
}

static mtr_value peek(struct mtr_vm* vm, size_t distance) {
    return *(vm->stack_top - distance - 1);
}

static mtr_value pop(struct mtr_vm* vm) {
    return *(--vm->stack_top);
}

static void push(struct mtr_vm* vm, mtr_value value) {
    *(vm->stack_top++) = value;
}

static u8* execute_instruction(struct mtr_vm* vm, u8* ip) {

#define BINARY_OP(op, type)                                            \
    do {                                                               \
        const mtr_value r = pop(vm);                                   \
        const mtr_value l = pop(vm);                                   \
        const mtr_value res = { .type = l.type op r.type };            \
        push(vm, res);                                                 \
    } while (false)

#define READ(type) *((type*)ip); ip += sizeof(type)

    switch (*ip++)
    {
        case MTR_OP_RETURN:
            break;

        case MTR_OP_INT: {
            const i64 value = READ(i64);
            const mtr_value constant = MTR_INT_VAL(value);
            push(vm, constant);
            break;
        }

        case MTR_OP_FLOAT: {
            const f64 value = READ(f64);
            const mtr_value constant = MTR_FLOAT_VAL(value);
            push(vm, constant);
            break;
        }

        case MTR_OP_FALSE: {
            const mtr_value c = MTR_INT_VAL(0);
            push(vm , c);
            break;
        }

        case MTR_OP_TRUE: {
            const mtr_value c = MTR_INT_VAL(1);
            push(vm , c);
            break;
        }

        case MTR_OP_NIL: {
            const mtr_value c = MTR_NIL;
            push(vm, c);
            break;
        }

        case MTR_OP_NOT: {
            const mtr_value value = pop(vm);
            const mtr_value res = MTR_INT_VAL(!value.integer);
            push(vm, res);
            break;
        }

        case MTR_OP_NEGATE_I: {
            const mtr_value value = pop(vm);
            const mtr_value res = MTR_INT_VAL(-value.integer);
            push(vm, res);
            break;
        }

        case MTR_OP_NEGATE_F: {
            const mtr_value value = pop(vm);
            const mtr_value res = MTR_FLOAT_VAL(-value.floating);
            push(vm, res);
            break;
        }

        case MTR_OP_ADD_I: BINARY_OP(+, integer); break;
        case MTR_OP_SUB_I: BINARY_OP(-, integer); break;
        case MTR_OP_MUL_I: BINARY_OP(*, integer); break;
        case MTR_OP_DIV_I: BINARY_OP(/, integer); break;

        case MTR_OP_ADD_F: BINARY_OP(+, floating); break;
        case MTR_OP_SUB_F: BINARY_OP(-, floating); break;
        case MTR_OP_MUL_F: BINARY_OP(*, floating); break;
        case MTR_OP_DIV_F: BINARY_OP(/, floating); break;

        case MTR_OP_GET: {
            const u16 index = READ(u16);
            push(vm, vm->stack[index]);
            break;
        }

        case MTR_OP_SET: {
            const u16 index = READ(u16);
            vm->stack[index] = pop(vm);
            break;
        }

        case MTR_OP_JMP: {
            const i16 where = READ(i16);
            ip += where;
            break;
        }

        case MTR_OP_JMP_Z: {
            const mtr_value value = peek(vm , 0);
            const bool con = MTR_AS_INT(value);
            const i16 where = READ(i16);
            if (!con) {
                ip += where;
            }
            break;
        }

        case MTR_OP_POP: {
            pop(vm);
            break;
        }

        case MTR_OP_END_SCOPE: {
            const u16 where = READ(u16);
            vm->stack_top -= where;
            break;
        }

        default:
            break;
    }

    return ip;

#undef BINARY_OP
#undef READ
}

static i32 run(struct mtr_vm* vm) {
    vm->ip = vm->chunk->bytecode;
    vm->stack_top = vm->stack;
    u8* ip = vm->ip;
    while (ip != vm->chunk->bytecode + vm->chunk->size) {
#ifndef NDEBUG
        // mtr_dump_stack(vm->stack, vm->stack_top);
        // mtr_disassemble_instruction(ip, ip - vm->chunk->bytecode);
#endif
        ip = execute_instruction(vm, ip);
    }
    vm->ip = ip;
    return pop(vm).integer;
}

i32 mtr_execute(struct mtr_vm* vm, struct mtr_package* package) {
    vm->chunk = mtr_package_get_chunk_by_name(package, "main");
    if (NULL == vm->chunk) {
        MTR_LOG_ERROR("Did not found main.");
        return false;
    }
    // mtr_disassemble(*vm->chunk, "while");
    // exit(-1);
    return run(vm);
}
