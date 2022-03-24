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

#define BINARY_OP(op, type)                                            \
    do {                                                               \
        const mtr_value r = pop(engine);                               \
        const mtr_value l = pop(engine);                               \
        const mtr_value res = { .type = l.type op r.type };            \
        push(engine, res);                                             \
    } while (false)

#define READ(type) *((type*)ip); ip += sizeof(type)
#define AS(type, value) *((type*)&value)

void mtr_call(struct mtr_engine* engine, const struct mtr_chunk chunk, u8 argc) {
    mtr_value* frame = engine->stack_top - argc;
    register u8* ip = chunk.bytecode;
    u8* end = chunk.bytecode + chunk.size;
    while (ip < end) {

        mtr_dump_stack(engine->stack, engine->stack_top);
        mtr_disassemble_instruction(ip, ip - chunk.bytecode);

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

            case MTR_OP_STRING_LITERAL: {
                const char* string = READ(const char*);
                // MTR_LOG_TRACE("%p", (void*)string);
                u32 length = READ(u32);
                i64 s = AS(i64, string);
                i64 l = AS(i64, length);
                push(engine, MTR_INT_VAL(s));
                push(engine, MTR_INT_VAL(l));
                break;
            }

            case MTR_OP_NIL: {
                const mtr_value c = MTR_NIL;
                push(engine, c);
                break;
            }

            case MTR_OP_NEW_STRING: {
                u32 length = 0;
                const char* string = NULL;
                mtr_value l = pop(engine);
                length = AS(u32, l.integer);
                if (length > 0) {
                    mtr_value s = pop(engine);
                    string = AS(const char*, s.integer);
                    // MTR_LOG_TRACE("%p", (void*)string);
                }

                struct mtr_string* m_string = mtr_new_string(string, length);
                struct mtr_object* o = (struct mtr_object*) m_string;

                push(engine, MTR_OBJ_VAL(o));
                break;
            }

            case MTR_OP_NEW_ARRAY: {
                struct mtr_array* array = mtr_new_array();

                i64 count = pop(engine).integer;

                for (i64 i = 0; i < count; ++i) {
                    const mtr_value elem = pop(engine);
                    mtr_array_append(array, elem);
                }

                struct mtr_object* o = (struct mtr_object*) array;
                const mtr_value v = MTR_OBJ_VAL(o);
                push(engine, v);
                break;
            }

            case MTR_OP_NEW_MAP: {
                struct mtr_map* map = mtr_new_map();
                struct mtr_object* o = (struct mtr_object*) map;
                const mtr_value v = MTR_OBJ_VAL(o);
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
                push(engine, frame[index]);
                break;
            }

            case MTR_OP_SET: {
                const u16 index = READ(u16);
                frame[index] = pop(engine);
                break;
            }

            case MTR_OP_GET_O: {
                const mtr_value key = pop(engine);
                const struct mtr_object* object = MTR_AS_OBJ(pop(engine));
                switch (object->type) {
                case MTR_OBJ_STRING: {
                    const struct mtr_string* string = (const struct mtr_string*) object;
                    const i64 i = MTR_AS_INT(key);
                    const size_t index = AS(size_t, i);
                    if (index >= string->length) {
                        IMPLEMENT // runtime error;
                        MTR_LOG_ERROR("Indexing array of size %zu with index %zu", string->length, index);
                        exit(-1);
                        break;
                    }
                    // need to think whether to malloc a whole new string for a single char or not.
                    // I dont like the idea. I could have a reference to it
                    MTR_LOG_ERROR("String indexing not yet implemented");
                    exit(-1);
                    break;
                }
                case MTR_OBJ_ARRAY: {
                    const struct mtr_array* array = (const struct mtr_array*) object;
                    const i64 i = MTR_AS_INT(key);
                    const size_t index = AS(size_t, i);
                    if (index >= array->size) {
                        IMPLEMENT // runtime error;
                        MTR_LOG_ERROR("Indexing array of size %zu with index %zu", array->size, index);
                        exit(-1);
                        break;
                    }
                    push(engine, array->elements[index]);
                    break;
                }
                case MTR_OBJ_MAP: {
                    struct mtr_map* map = (struct mtr_map*) object;
                    mtr_value val = mtr_map_get(map, key);
                    push(engine, val);
                    break;
                }
                default:
                    IMPLEMENT // runtime error
                    exit(-1);
                    break;
                }
                break;
            }

            case MTR_OP_SET_O: {
                const mtr_value key = pop(engine);
                const struct mtr_object* object = MTR_AS_OBJ(pop(engine));
                mtr_value val = pop(engine);
                switch (object->type) {
                case MTR_OBJ_STRING: {
                    MTR_LOG_ERROR("<String> object does not support item assignment.");
                    exit(-1);
                    break;
                }
                case MTR_OBJ_ARRAY: {
                    const struct mtr_array* array = (const struct mtr_array*) object;
                    const i64 i = MTR_AS_INT(key);
                    const size_t index = AS(size_t, i);
                    if (index >= array->size) {
                        IMPLEMENT // runtime error;
                        MTR_LOG_ERROR("Indexing array of size %zu with index %zu", array->size, index);
                        exit(-1);
                        break;
                    }
                    array->elements[index] = val;
                    break;
                }
                case MTR_OBJ_MAP: {
                    struct mtr_map* map = (struct mtr_map*) object;
                    mtr_map_insert(map, key, val);
                    break;
                }
                default:
                    IMPLEMENT // runtime error
                    exit(-1);
                    break;
                }
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
                if (!condition) {
                    ip += where;
                }
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
                const u8 argc = READ(u8);
                struct mtr_object* obj = MTR_AS_OBJ(pop(engine));
                struct mtr_invokable* i = (struct mtr_invokable*) obj;
                i->call(obj, engine, argc);
                break;
            }

            case MTR_OP_RETURN: {
                mtr_value res = pop(engine);
                engine->stack_top = frame;
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

i32 mtr_execute(struct mtr_engine* engine, struct mtr_package* package) {
    engine->package = package;
    engine->stack_top = engine->stack;
    struct mtr_object* o = mtr_package_get_function_by_name(engine->package, "main");
    if (NULL == o) {
        MTR_LOG_ERROR("Did not find main.");
        return -1;
    }

    for (size_t i = 0; i < package->count; ++i) {
        struct mtr_object* o = package->functions[i];
        push(engine, MTR_OBJ_VAL(o));
    }

    struct mtr_function* f = (struct mtr_function*) o;
    mtr_call(engine, f->chunk, package->count);

    // mtr_dump_stack(engine->stack, engine->stack_top);
    return 0;
}
