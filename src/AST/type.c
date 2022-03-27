#include "type.h"

#include "core/log.h"
#include "debug/dump.h"
#include "scanner/token.h"

#include <stdlib.h>
#include <string.h>

const struct mtr_type invalid_type = {
    .type = MTR_DATA_INVALID,
    .obj = NULL
};

struct mtr_type mtr_get_data_type(struct mtr_token type) {
    struct mtr_type t = invalid_type;
    switch (type.type)
    {
    case MTR_TOKEN_INT_LITERAL:
    case MTR_TOKEN_INT: {
        t.type = MTR_DATA_INT;
        break;
    }

    case MTR_TOKEN_FLOAT_LITERAL:
    case MTR_TOKEN_FLOAT: {
        t.type = MTR_DATA_FLOAT;
        break;
    }

    case MTR_TOKEN_BOOL:
    case MTR_TOKEN_TRUE:
    case MTR_TOKEN_FALSE: {
        t.type = MTR_DATA_BOOL;
        break;
    }

    case MTR_TOKEN_STRING_LITERAL:
    case MTR_TOKEN_STRING: {
        t.type = MTR_DATA_STRING;
        break;
    }

    case MTR_TOKEN_IDENTIFIER: {
        t.type = MTR_DATA_USER_DEFINED;
        break;
    }

    case MTR_TOKEN_ANY: {
        t.type = MTR_DATA_ANY;
        break;
    }

    default:
        MTR_LOG_DEBUG("Invalid data type  %s", mtr_token_type_to_str(type.type));
        break;
    }
    return t;
}

struct mtr_type mtr_copy_type(struct mtr_type type) {
    switch (type.type) {
    case MTR_DATA_USER_DEFINED: MTR_ASSERT(false, "user types not implemented yet.");
    case MTR_DATA_ARRAY: {
        struct mtr_array_type* a = (struct mtr_array_type*) type.obj;
        return mtr_new_array_type(a->type);
    }
    case MTR_DATA_MAP: {
        struct mtr_map_type* m = (struct mtr_map_type*) type.obj;
        return mtr_new_map_type(m->key, m->value);
    }
    case MTR_DATA_FN: {
        struct mtr_function_type* f = (struct mtr_function_type*) type.obj;
        return mtr_new_function_type(f->return_, f->argc, f->argv);
    }
    default:
        return type; // no need to copy anythin;
    }

}

static void delete_object_type(mtr_object_type* obj, enum mtr_data_type type) {
    switch (type) {
    case MTR_DATA_USER_DEFINED: return;
    case MTR_DATA_ARRAY: {
        struct mtr_array_type* a = (struct mtr_array_type*) obj;
        mtr_delete_type(a->type);
        free(a);
        return;
    }
    case MTR_DATA_MAP: {
        struct mtr_map_type* m = (struct mtr_map_type*) obj;
        mtr_delete_type(m->key);
        mtr_delete_type(m->value);
        free(m);
        return;
    }
    case MTR_DATA_FN: {
        struct mtr_function_type* f = (struct mtr_function_type*) obj;
        mtr_delete_type(f->return_);
        for (u8 i = 0; i < f->argc; ++i) {
            mtr_delete_type(f->argv[i]);
        }
        free(f->argv);
        free(f);
        return;
    }
    default:
        break;
    }
    MTR_ASSERT(false, "Invalid type.");
}

void mtr_delete_type(struct mtr_type type) {
    if (type.obj) {
        delete_object_type(type.obj, type.type);
    }
}

static bool object_type_match(mtr_object_type* lhs, mtr_object_type* rhs, enum mtr_data_type type) {
    switch (type) {
    case MTR_DATA_INVALID: return false;
    case MTR_DATA_USER_DEFINED: return false;
    case MTR_DATA_ARRAY: {
        struct mtr_array_type* l = (struct mtr_array_type*) lhs;
        struct mtr_array_type* r = (struct mtr_array_type*) rhs;
        return mtr_type_match(l->type, r->type);
    }
    case MTR_DATA_MAP: {
        struct mtr_map_type* l = (struct mtr_map_type*) lhs;
        struct mtr_map_type* r = (struct mtr_map_type*) rhs;
        return mtr_type_match(l->key, r->key) && mtr_type_match(l->value, r->value);
    }
    case MTR_DATA_FN: {
        struct mtr_function_type* l = (struct mtr_function_type*) lhs;
        struct mtr_function_type* r = (struct mtr_function_type*) rhs;
        return mtr_type_match(l->return_, r->return_);
    }
    default:
        break;
    }
    MTR_ASSERT(false, "Invalid data type");
    return false;
}

bool mtr_type_match(struct mtr_type lhs, struct mtr_type rhs) {
    bool any = lhs.type == MTR_DATA_ANY || rhs.type == MTR_DATA_ANY;
    bool match = (lhs.type == rhs.type)
            && (
                (lhs.type >= MTR_DATA_BOOL) || object_type_match(lhs.obj, rhs.obj, lhs.type)
            // relying on short circuiting to decide whether to call object_type_match or not
            );
    return any || match;
}


struct mtr_type mtr_get_underlying_type(struct mtr_type type) {
    switch (type.type) {
    case MTR_DATA_ARRAY: {
        struct mtr_array_type* a = (struct mtr_array_type*) type.obj;
        return a->type;
    }
    case MTR_DATA_MAP: {
        struct mtr_map_type* m = (struct mtr_map_type*) type.obj;
        return m->value;
    }
    case MTR_DATA_FN: {
        struct mtr_function_type* f = (struct mtr_function_type*) type.obj;
        return f->return_;
    }
    default:
        return invalid_type;
    }
}

struct mtr_type mtr_new_array_type(struct mtr_type type) {
    struct mtr_array_type* a = malloc(sizeof(*a));
    a->type = type;

    struct mtr_type t;
    t.type = MTR_DATA_ARRAY;
    t.obj = a;
    return t;
}


struct mtr_type mtr_new_map_type(struct mtr_type key, struct mtr_type value) {
    struct mtr_map_type* m = malloc(sizeof(*m));
    m->key = key;
    m->value = value;

    struct mtr_type t;
    t.type = MTR_DATA_MAP;
    t.obj = m;
    return t;
}

struct mtr_type mtr_new_function_type(struct mtr_type return_, u8 argc, struct mtr_type* argv) {
    struct mtr_function_type* f = malloc(sizeof(*f));
    f->return_ = return_;
    f->argc = argc;
    f->argv = NULL;
    if (argc > 0) {
        f->argv = malloc(sizeof(struct mtr_type) * argc);
        memcpy(f->argv, argv, sizeof(struct mtr_type) * argc);
    }

    struct mtr_type t;
    t.type = MTR_DATA_FN;
    t.obj = f;
    return t;
}
