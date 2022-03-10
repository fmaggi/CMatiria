#include "type.h"

#include "core/log.h"

#include <stdlib.h>

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
    default:
        break;
    }
    MTR_ASSERT(false, "Invalid type.");
}

void mtr_delete_type(struct mtr_type type) {
    if (type.obj) {
        delete_object_type(type.obj, type.type);
        type.obj = NULL;
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
    default:
        return invalid_type;
    }
}

struct mtr_array_type* mtr_new_array_obj(struct mtr_type type) {
    struct mtr_array_type* a = malloc(sizeof(*a));
    a->type = type;
    return a;
}
