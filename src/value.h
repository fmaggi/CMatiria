#ifndef _MTR_VALUE_H
#define _MTR_VALUE_H

#include "core/types.h"

typedef union {
    i64 integer;
    f64 floating;
    bool boolean;
    char* string;
} mtr_value;

#define MTR_INT_VAL(value)   { .integer = value }
#define MTR_FLOAT_VAL(value) { .floating = value }
#define MTR_BOOL_VAL(value)  { .boolean = value }

#define MTR_AS_INT(value)   value.integer
#define MTR_AS_FLOAT(value) value.floating
#define MTR_AS_BOOL(value)  value.boolean

#define MTR_NIL MTR_INT_VAL(0)

#endif
