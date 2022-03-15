#include "type.h"

#include "core/log.h"
#include "debug/dump.h"

#include <stdlib.h>
#include <string.h>

const struct mtr_type invalid_type = {
    .type = MTR_DATA_INVALID,
    .obj = NULL
};

static void delete_object_type(mtr_object_type* obj, enum mtr_data_type type) {
    switch (type) {
    case MTR_DATA_USER_DEFINED: return;
    case MTR_DATA_ARRAY: {
        struct mtr_array_type* a = (struct mtr_array_type*) obj;
        mtr_delete_type(a->type);
        free(a);
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
    return (lhs.type == rhs.type)
        && (
            (lhs.type >= MTR_DATA_BOOL) || object_type_match(lhs.obj, rhs.obj, lhs.type)
            // relying on short circuiting to decide whether to call object_type_match or not
        );
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

struct mtr_array_type* mtr_new_array_type(struct mtr_type type) {
    struct mtr_array_type* a = malloc(sizeof(*a));
    a->type = type;
    return a;
}


struct mtr_map_type* mtr_new_map_type(struct mtr_type key, struct mtr_type value) {
    struct mtr_map_type* m = malloc(sizeof(*m));
    m->key = key;
    m->value = value;
    return m;
}

struct mtr_function_type* mtr_new_function_type(struct mtr_type return_, u8 argc, struct mtr_type* argv) {
    struct mtr_function_type* f = malloc(sizeof(*f));
    f->return_ = return_;
    f->argc = argc;
    f->argv = NULL;
    if (argc > 0) {
        f->argv = malloc(sizeof(struct mtr_type) * argc);
        memcpy(f->argv, argv, sizeof(struct mtr_type) * argc);
    }
    return f;
}
