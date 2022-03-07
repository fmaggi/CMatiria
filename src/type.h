#ifndef MTR_TYPE_H
#define MTR_TYPE_H

#include "core/types.h"


enum mtr_data_type {
    MTR_DATA_INVALID = 0,

    MTR_DATA_USER_DEFINED,
    MTR_DATA_ARRAY,

    MTR_DATA_BOOL,
    MTR_DATA_INT,
    MTR_DATA_FLOAT,
};

struct mtr_object_type {
};

struct mtr_type {
    struct mtr_object_type* obj;
    enum mtr_data_type type;
};

void mtr_delete_type(struct mtr_type type);

bool mtr_type_match(struct mtr_type lhs, struct mtr_type rhs);

struct mtr_array_type {
    struct mtr_object_type obj;
    struct mtr_type type;
};

struct mtr_array_type* mtr_new_array_obj(struct mtr_type type);

#endif
