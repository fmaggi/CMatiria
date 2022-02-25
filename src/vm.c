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
        const mtr_value res = { .integer = l.type op r.type };         \
        push(vm, res);                                                 \
    } while (false)

    switch (*ip++)
    {
        case MTR_OP_RETURN:
            mtr_print_value(pop(vm));
            break;
        case MTR_OP_CONSTANT: {
            const mtr_value constant = { .integer =  *((u64*)ip) };
            ip += 8;
            push(vm, constant);
            break;
        }
        case MTR_OP_PLUS_I:  BINARY_OP(+, integer); break;
        case MTR_OP_MINUS_I: BINARY_OP(-, integer); break;
        case MTR_OP_MUL_I:   BINARY_OP(*, integer); break;
        case MTR_OP_DIV_I:   BINARY_OP(/, integer); break;
        default:
            break;
    }

    return ip;

#undef BINARY_OP
}

bool mtr_execute(struct mtr_vm* vm, struct mtr_chunk* chunk) {
    vm->chunk = chunk;
    vm->ip = chunk->bytecode;
    vm->stack_top = vm->stack;
    while (vm->ip != chunk->bytecode + chunk->size) {
        vm->ip = execute_instruction(vm, vm->ip);
    }
    return true;
}

void mtr_print_value(mtr_value value) {
    MTR_LOG_DEBUG("%lu", value.integer);
}
