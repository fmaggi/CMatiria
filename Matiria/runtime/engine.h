#ifndef MTR_VM_H
#define MTR_VM_H

#include "value.h"
#include "bytecode.h"
#include "package.h"

#include "core/types.h"

#define MTR_MAX_STACK 1024

struct mtr_engine {
    mtr_value stack[MTR_MAX_STACK];
    mtr_value* stack_top;
    struct mtr_object** globals;
};

i32 mtr_execute(struct mtr_engine* engine, struct mtr_package* package);

#endif
