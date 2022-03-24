#ifndef MTR_VALUE_H
#define MTR_VALUE_H

#include "core/types.h"

enum mtr_value_type {
    MTR_VAL_INT,
    MTR_VAL_FLOAT,
    MTR_VAL_OBJ
};

typedef struct {
    enum mtr_value_type type;
    u32 pad; // I have an extra four bytes use because of alignment
    union {
        i64 integer;
        f64 floating;
        struct mtr_object* object;
    };
} mtr_value;

#define MTR_INT_VAL(value)   (mtr_value){ .integer = value, .type = MTR_VAL_INT }
#define MTR_FLOAT_VAL(value) (mtr_value){ .floating = value, .type = MTR_VAL_FLOAT }
#define MTR_OBJ_VAL(value)   (mtr_value){ .object = value, .type = MTR_VAL_OBJ }

#define MTR_AS_INT(value)   value.integer
#define MTR_AS_FLOAT(value) value.floating
#define MTR_AS_OBJ(value)   value.object

#define MTR_NIL MTR_INT_VAL(0)

#endif
