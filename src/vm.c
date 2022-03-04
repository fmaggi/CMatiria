#include "vm.h"

#include "bytecode.h"
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
    if (vm->stack_top == vm->stack + MTR_MAX_STACK) {
        MTR_LOG_ERROR("Stack overflow.");
        exit(-1);
    }
    *(vm->stack_top++) = value;
}

#define BINARY_OP(op, type)                                            \
    do {                                                               \
        const mtr_value r = pop(vm);                                   \
        const mtr_value l = pop(vm);                                   \
        const mtr_value res = { .type = l.type op r.type };            \
        push(vm, res);                                                 \
    } while (false)

#define READ(type) *((type*)ip); ip += sizeof(type)

struct call_frame {
    mtr_value* stack;
};

static void call(struct mtr_vm* vm, const struct mtr_chunk* chunk, u8 argc) {
    struct call_frame frame;
    frame.stack = vm->stack_top - argc;
    register u8* ip = chunk->bytecode;
    u8* end = chunk->bytecode + chunk->size;
    while (ip < end) {
        mtr_dump_stack(frame.stack, vm->stack_top);
        mtr_disassemble_instruction(ip, ip - chunk->bytecode);

        switch (*ip++)
        {
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

            case MTR_OP_LESS_I: BINARY_OP(<, integer); MTR_PROFILE_FUNC(); break;
            case MTR_OP_GREATER_I: BINARY_OP(>, integer); break;
            case MTR_OP_EQUAL_I: BINARY_OP(==, integer); break;

            case MTR_OP_GET: {
                const u16 index = READ(u16);
                push(vm, frame.stack[index]);
                break;
            }

            case MTR_OP_SET: {
                const u16 index = READ(u16);
                frame.stack[index] = pop(vm);
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

            case MTR_OP_POP_V: {
                const u16 count = READ(u16);
                vm->stack_top -= count;
                break;
            }

            case MTR_OP_CALL: {
                const u16 index = READ(u16);
                const u8 argc = READ(u8);
                struct mtr_chunk* chunk = vm->package->functions + index;
                call(vm, chunk, argc);
                break;
            }

            case MTR_OP_RETURN: {
                mtr_value res = pop(vm);
                vm->stack_top = frame.stack;
                push(vm, res);
                return;
            }

            default:
                break;
        }
    }
}

#undef BINARY_OP
#undef READ

i32 mtr_execute(struct mtr_package* package) {
    struct mtr_vm vm;
    vm.package = package;
    vm.stack_top = vm.stack;
    struct mtr_chunk* main_chunk = mtr_package_get_chunk_by_name(vm.package, "main");
    if (NULL == main_chunk) {
        MTR_LOG_ERROR("Did not find main.");
        return -1;
    }

    vm.stack_top = vm.stack + 2;
    call(&vm, main_chunk, 2);
    return 0;
}
