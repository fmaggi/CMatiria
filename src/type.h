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

struct mtr_object {
};

struct mtr_type {
    struct mtr_object* obj;
    enum mtr_data_type type;
};

struct mtr_array {
    struct mtr_object obj;
    struct mtr_type type;
};

struct mtr_object* mtr_new_array_obj(struct mtr_type type);

#endif
