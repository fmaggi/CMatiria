#include "engine.h"

#include "bytecode.h"
#include "core/log.h"

#include "object.h"

#include "debug/disassemble.h"
#include "value.h"

#include "core/macros.h"

static mtr_value peek(struct mtr_engine* engine, size_t distance) {
    return *(engine->stack_top - distance - 1);
}

static mtr_value pop(struct mtr_engine* engine) {
    return *(--engine->stack_top);
}

static void push(struct mtr_engine* engine, mtr_value value) {
#ifndef NDEBUG
    if (engine->stack_top == engine->stack + MTR_MAX_STACK) {
        MTR_LOG_ERROR("Stack overflow.");
        exit(-1);
    }
#endif
    *(engine->stack_top++) = value;
}

static void call(struct mtr_engine* engine, const struct mtr_chunk chunk, u8 argc);

static void invoke(struct mtr_engine* engine, struct mtr_object* object, u8 argc) {
    if (object->type == MTR_OBJ_FUNCTION) {
        struct mtr_function* f = (struct mtr_function*) object;
        call(engine, f->chunk, argc);
        return;
    } else if (object->type == MTR_OBJ_NATIVE_FN) {
        struct mtr_native_fn* n = (struct mtr_native_fn*) object;
        mtr_value val = n->function(argc, engine->stack_top - argc);
        engine->stack_top -= argc;
        push(engine, val);
        return;
    }
    MTR_ASSERT(false, "Object is not invokable");
}

#define BINARY_OP(op, type)                                            \
    do {                                                               \
        const mtr_value r = pop(engine);                               \
        const mtr_value l = pop(engine);                               \
        const mtr_value res = { .type = l.type op r.type };            \
        push(engine, res);                                             \
    } while (false)

#define READ(type) *((type*)ip); ip += sizeof(type)

static void call(struct mtr_engine* engine, const struct mtr_chunk chunk, u8 argc) {
    mtr_value* frame = engine->stack_top - argc;
    register u8* ip = chunk.bytecode;
    u8* end = chunk.bytecode + chunk.size;
    while (ip < end) {

        // mtr_dump_stack(frame, engine->stack_top);
        // mtr_disassemble_instruction(ip, ip - chunk.bytecode);

        switch (*ip++)
        {
            case MTR_OP_INT: {
                const i64 value = READ(i64);
                const mtr_value constant = MTR_INT(value);
                push(engine, constant);
                break;
            }

            case MTR_OP_FLOAT: {
                const f64 value = READ(f64);
                const mtr_value constant = MTR_FLOAT(value);
                push(engine, constant);
                break;
            }

            case MTR_OP_FALSE: {
                const mtr_value c = MTR_INT(0);
                push(engine , c);
                break;
            }

            case MTR_OP_TRUE: {
                const mtr_value c = MTR_INT(1);
                push(engine , c);
                break;
            }

            case MTR_OP_STRING_LITERAL: {
                const char* string = READ(const char*);
                u32 length = READ(u32);
                struct mtr_string* s = mtr_new_string(string, length);
                push(engine, MTR_OBJ(s));
                break;
            }

            case MTR_OP_ARRAY_LITERAL: {
                struct mtr_array* array = mtr_new_array();

                u8 count = READ(u8);

                for (u8 i = 0; i < count; ++i) {
                    const mtr_value elem = pop(engine);
                    mtr_array_append(array, elem);
                }

                push(engine, MTR_OBJ(array));
                break;
            }

            case MTR_OP_MAP_LITERAL: {
                struct mtr_map* map = mtr_new_map();

                u8 count = READ(u8);

                for (u8 i = 0; i < count; ++i) {
                    const mtr_value value = pop(engine);
                    const mtr_value key = pop(engine);
                    mtr_map_insert(map, key, value);
                }

                push(engine, MTR_OBJ(map));
                break;
            }

            case MTR_OP_CONSTRUCTOR: {
                u8 count = READ(u8);
                struct mtr_struct* s = mtr_new_struct(count);
                for (u8 i = 0; i < count; ++i) {
                    u8 actual_index = count - i - 1;
                    s->members[actual_index] = pop(engine);
                }
                push(engine, MTR_OBJ(s));
                break;
            }

            case MTR_OP_NIL: {
                const mtr_value c = MTR_NIL;
                push(engine, c);
                break;
            }

            case MTR_OP_STRING: {
                mtr_value string = peek(engine, 0);
                if (NULL == string.object) {
                    pop(engine); // pop null value;
                    struct mtr_string* string_object = mtr_new_string(NULL, 0);
                    push(engine, MTR_OBJ(string_object));
                }
                break;
            }

            case MTR_OP_ARRAY: {
                mtr_value array = peek(engine, 0);
                if (NULL == array.object) {
                    pop(engine); // pop null value;
                    struct mtr_array* array_object = mtr_new_array();
                    push(engine, MTR_OBJ(array_object));
                }
                break;
            }

            case MTR_OP_MAP: {
                mtr_value map = peek(engine, 0);
                if (NULL == map.object) {
                    pop(engine); // pop null value;
                    struct mtr_map* map = mtr_new_map();
                    push(engine, MTR_OBJ(map));
                }
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

            case MTR_OP_GLOBAL_GET: {
                const u16 index = READ(u16);
                struct mtr_object* o = engine->package->globals[index];
                push(engine, MTR_OBJ(o));
                break;
            }

            case MTR_OP_SET: {
                const u16 index = READ(u16);
                frame[index] = pop(engine);
                break;
            }

            case MTR_OP_INDEX_GET: {
                const mtr_value key = pop(engine);
                const struct mtr_object* object = MTR_AS_OBJ(pop(engine));
                switch (object->type) {
                case MTR_OBJ_STRING: {
                    const struct mtr_string* string = (const struct mtr_string*) object;
                    const i64 i = MTR_AS_INT(key);
                    const size_t index = mtr_reinterpret_cast(size_t, i);
                    if (index >= string->length) {
                        IMPLEMENT // runtime error;
                        MTR_LOG_ERROR("Indexing string of size %zu with index %zu", string->length, index);
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
                    const size_t index = mtr_reinterpret_cast(size_t, i);
                    if (index >= array->size) {
                        IMPLEMENT // runtime error;
                        MTR_LOG_ERROR("Out of bounds: Indexing array of size %zu with index %zu", array->size, index);
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

            case MTR_OP_INDEX_SET: {
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
                    const size_t index = mtr_reinterpret_cast(size_t, i);
                    if (index >= array->size) {
                        IMPLEMENT // runtime error;
                        MTR_LOG_ERROR("Out of bounds: Indexing array of size %zu with index %zu", array->size, index);
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
                    MTR_ASSERT(false, "Invalid object type");
                    break;
                }
                break;
            }

            case MTR_OP_STRUCT_GET: {
                const mtr_value v = pop(engine);
                const struct mtr_struct* s = (const struct mtr_struct*) MTR_AS_OBJ(v);
                const u8 index = READ(u16);
                push(engine, s->members[index]);
                break;
            }

            case MTR_OP_STRUCT_SET: {
                mtr_value k = pop(engine);
                mtr_value val = pop(engine);
                struct mtr_struct* s = (struct mtr_struct*) MTR_AS_OBJ(k);
                const u8 index = READ(u16);
                s->members[index] = val;
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
                invoke(engine, obj, argc);
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
                const mtr_value to = MTR_INT((i64) from.floating);
                push(engine, to);
                break;
            }

            case MTR_OP_FLOAT_CAST: {
                const mtr_value from = pop(engine);
                const mtr_value to = MTR_FLOAT((f64) from.integer);
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

    struct mtr_function* f = (struct mtr_function*) o;

    call(engine, f->chunk, 0);

    // mtr_dump_stack(engine->stack, engine->stack_top);
    return 0;
}
