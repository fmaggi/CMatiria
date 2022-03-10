#include "engine.h"

#include "bytecode.h"
#include "core/log.h"

#include "object.h"

#include "debug/disassemble.h"
#include "value.h"

void dump_value(mtr_value value, enum mtr_data_type type) {
    if (type == MTR_DATA_FLOAT) {
        MTR_LOG_DEBUG("%.3f", value.floating);
    } else {
        MTR_LOG_DEBUG("%li", value.integer);
    }
}

static mtr_value peek(struct mtr_engine* engine, size_t distance) {
    return *(engine->stack_top - distance - 1);
}

mtr_value mtr_pop(struct mtr_engine* engine) {
    return *(--engine->stack_top);
}

void mtr_push(struct mtr_engine* engine, mtr_value value) {
    if (engine->stack_top == engine->stack + MTR_MAX_STACK) {
        MTR_LOG_ERROR("Stack overflow.");
        exit(-1);
    }
    *(engine->stack_top++) = value;
}

static mtr_value pop(struct mtr_engine* engine) {
    return mtr_pop(engine);
}

static void push(struct mtr_engine* engine, mtr_value value) {
    mtr_push(engine, value);
}

struct call_frame {
    mtr_value* stack;
};

#define BINARY_OP(op, type)                                            \
    do {                                                               \
        const mtr_value r = pop(engine);                                   \
        const mtr_value l = pop(engine);                                   \
        const mtr_value res = { .type = l.type op r.type };            \
        push(engine, res);                                                 \
    } while (false)

#define READ(type) *((type*)ip); ip += sizeof(type)
#define AS(type, value) *((type*)&value)

void mtr_call(struct mtr_engine* engine, const struct mtr_chunk chunk, u8 argc) {
    struct call_frame frame;
    frame.stack = engine->stack_top - argc;
    register u8* ip = chunk.bytecode;
    u8* end = chunk.bytecode + chunk.size;
    while (ip < end) {
        // mtr_dump_stack(frame.stack, engine->stack_top);
        // mtr_disassemble_instruction(ip, ip - chunk.bytecode);

        switch (*ip++)
        {
            case MTR_OP_INT: {
                const i64 value = READ(i64);
                const mtr_value constant = MTR_INT_VAL(value);
                push(engine, constant);
                break;
            }

            case MTR_OP_FLOAT: {
                const f64 value = READ(f64);
                const mtr_value constant = MTR_FLOAT_VAL(value);
                push(engine, constant);
                break;
            }

            case MTR_OP_FALSE: {
                const mtr_value c = MTR_INT_VAL(0);
                push(engine , c);
                break;
            }

            case MTR_OP_TRUE: {
                const mtr_value c = MTR_INT_VAL(1);
                push(engine , c);
                break;
            }

            case MTR_OP_NIL: {
                const mtr_value c = MTR_NIL;
                push(engine, c);
                break;
            }

            case MTR_OP_NEW_ARRAY: {
                struct mtr_array* a = mtr_new_array();
                const mtr_value v = { .object = a };
                push(engine, v);
                break;
            }

            case MTR_OP_NOT: {
                (engine->stack_top - 1)->integer = !((engine->stack_top - 1)->integer);
                break;
            }

            case MTR_OP_OR: {
                const i16 where = READ(i16);
                const mtr_value condition = peek(engine, 0);
                if (condition.integer) {
                    ip += where;
                } else {
                    pop(engine);
                }
                break;
            }

            case MTR_OP_AND: {
                const i16 where = READ(i16);
                const mtr_value condition = peek(engine, 0);
                if (!condition.integer) {
                    ip += where;
                } else {
                    pop(engine);
                }
                break;
            }

            case MTR_OP_NEGATE_I: {
                (engine->stack_top - 1)->integer = -((engine->stack_top - 1)->integer);
                break;
            }

            case MTR_OP_NEGATE_F: {
                (engine->stack_top - 1)->floating = -((engine->stack_top - 1)->floating);
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

            case MTR_OP_LESS_I: BINARY_OP(<, integer); break;
            case MTR_OP_GREATER_I: BINARY_OP(>, integer); break;
            case MTR_OP_EQUAL_I: BINARY_OP(==, integer); break;

            case MTR_OP_LESS_F: BINARY_OP(<, floating); break;
            case MTR_OP_GREATER_F: BINARY_OP(>, floating); break;
            case MTR_OP_EQUAL_F: BINARY_OP(==, floating); break;

            case MTR_OP_GET: {
                const u16 index = READ(u16);
                push(engine, frame.stack[index]);
                break;
            }

            case MTR_OP_SET: {
                const u16 index = READ(u16);
                frame.stack[index] = pop(engine);
                break;
            }

            case MTR_OP_GET_A: {
                const i64 i = MTR_AS_INT(pop(engine));
                const size_t index = AS(size_t, i);
                const struct mtr_array* a = MTR_AS_OBJ(pop(engine));
                push(engine, a->elements[index]);
                break;
            }

            case MTR_OP_SET_A: {
                // const u16 index = READ(u16);
                // const u16 array_index = READ(u16);
                // const mtr_value val = frame.stack[index];
                // const struct mtr_array* a = val.object;
                // a->elements[array_index] = pop(engine);
                break;
            }

            case MTR_OP_JMP: {
                const i16 where = READ(i16);
                ip += where;
                break;
            }

            case MTR_OP_JMP_Z: {
                const mtr_value value = pop(engine);
                const bool condition = MTR_AS_INT(value);
                const i16 where = READ(i16);
                ip += where * !condition;
                break;
            }

            case MTR_OP_POP: {
                pop(engine);
                break;
            }

            case MTR_OP_POP_V: {
                const u16 count = READ(u16);
                engine->stack_top -= count;
                break;
            }

            case MTR_OP_CALL: {
                const u16 index = READ(u16);
                const u8 argc = READ(u8);

                struct mtr_object* val = engine->package->functions[index];
                struct mtr_invokable* i = (struct mtr_invokable*) val;
                i->call(val, engine, argc);
                break;
            }

            case MTR_OP_RETURN: {
                mtr_value res = pop(engine);
                engine->stack_top = frame.stack;
                push(engine, res);
                return;
            }

            case MTR_OP_INT_CAST: {
                const mtr_value from = pop(engine);
                const mtr_value to = MTR_INT_VAL((i64) from.floating);
                push(engine, to);
                break;
            }

            case MTR_OP_FLOAT_CAST: {
                const mtr_value from = pop(engine);
                const mtr_value to = MTR_FLOAT_VAL((f64) from.integer);
                push(engine, to);
                break;
            }
        }
    }
}

#undef BINARY_OP
#undef READ
#undef AS

i32 mtr_execute(struct mtr_engine* engine, struct mtr_package* package) {
    engine->package = package;
    engine->stack_top = engine->stack;
    struct mtr_object* o = mtr_package_get_function_by_name(engine->package, "main");
    if (NULL == o) {
        MTR_LOG_ERROR("Did not find main.");
        return -1;
    }
    struct mtr_function* f = (struct mtr_function*) o;
    mtr_call(engine, f->chunk, 0);

    // mtr_dump_stack(engine->stack, engine->stack_top);
    return 0;
}
