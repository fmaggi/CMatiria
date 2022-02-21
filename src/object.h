#ifndef _MTR_OBJECT_H
#define _MTR_OBJECT_H

#include "core/types.h"

enum mtr_object_type {
    MTR_OBJ_INT,
    MTR_OBJ_FLOAT,
    MTR_OBJ_BOOL,
};

struct mtr_object {
    union {
        u64 integer;
        f64 floating;
        bool boolean;
    } as;
    enum mtr_object_type type;
};

#endif
