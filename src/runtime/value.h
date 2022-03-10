#ifndef MTR_VALUE_H
#define MTR_VALUE_H

#include "core/types.h"

typedef union {
    i64 integer;
    f64 floating;
    void* object;
} mtr_value;

#define MTR_INT_VAL(value)   { .integer = value }
#define MTR_FLOAT_VAL(value) { .floating = value }
#define MTR_OBJ_VAL(value)   { .object = value }

#define MTR_AS_INT(value)   value.integer
#define MTR_AS_FLOAT(value) value.floating
#define MTR_AS_OBJ(value)   value.object

#define MTR_NIL MTR_INT_VAL(0)

#endif
