#include "vm.h"

#include "core/log.h"
#include "debug/disassemble.h"

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

#define READ(type) *((type*)((void*)ip)); ip += sizeof(type)

    switch (*ip++)
    {
        case MTR_OP_RETURN:
            mtr_print_value(pop(vm));
            break;

        case MTR_OP_INT: {
            i64 value = READ(i64);
            const mtr_value constant = MTR_INT_VAL(value);
            push(vm, constant);
            break;
        }

        case MTR_OP_FLOAT: {
            f64 value = READ(f64);
            const mtr_value constant = MTR_FLOAT_VAL(value);
            push(vm, constant);
            break;
        }

        case MTR_OP_NIL: {
            const mtr_value c = MTR_INT_VAL(0);
            push(vm, c);
            break;
        }

        case MTR_OP_PLUS_I:  BINARY_OP(+, integer); break;
        case MTR_OP_MINUS_I: BINARY_OP(-, integer); break;
        case MTR_OP_MUL_I:   BINARY_OP(*, integer); break;
        case MTR_OP_DIV_I:   BINARY_OP(/, integer); break;

        case MTR_OP_GET: {
            u16 index = READ(u16);
            push(vm, vm->stack[index]);
            break;
        }

        case MTR_OP_SET: {
            u16 index = READ(u16);
            vm->stack[index] = *vm->stack_top;
            break;
        }
        default:
            break;
    }

    return ip;

#undef BINARY_OP
#undef READ
}

static bool run(struct mtr_vm* vm) {
    vm->ip = vm->chunk->bytecode;
    vm->stack_top = vm->stack;
    while (vm->ip != vm->chunk->bytecode + vm->chunk->size) {
        vm->ip = execute_instruction(vm, vm->ip);
    }
    return true;
}

bool mtr_execute(struct mtr_vm* vm, struct mtr_package* package) {
    vm->chunk = mtr_package_get_chunk_by_name(package, "main");
    if (NULL == vm->chunk) {
        MTR_LOG_ERROR("Did not found main.");
        return false;
    }
    return run(vm);
}

void mtr_print_value(mtr_value value) {
    MTR_LOG_DEBUG("%li", value.integer);
}
