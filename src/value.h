#ifndef _MTR_VALUE_H
#define _MTR_VALUE_H

#include "core/types.h"

typedef union {
    u64 integer;
    f64 floating;
    char* string;
} mtr_value;

void mtr_print_value(mtr_value value);

#endif
