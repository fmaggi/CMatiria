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
    struct mtr_package* package;
};

i32 mtr_execute(struct mtr_engine* engine, struct mtr_package* package);

void mtr_call(struct mtr_engine* engine, const struct mtr_chunk chunk, u8 argc);

void mtr_push(struct mtr_engine* engine, mtr_value value);
mtr_value mtr_pop(struct mtr_engine* engine);



#endif
