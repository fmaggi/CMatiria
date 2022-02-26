#ifndef _MTR_VM_H
#define _MTR_VM_H

#include "value.h"
#include "bytecode.h"
#include "package.h"

#include "core/types.h"

#define MTR_MAX_STACK 1024

struct mtr_vm {
    mtr_value stack[MTR_MAX_STACK];
    mtr_value* stack_top;
    struct mtr_chunk* chunk;
    u8* ip;
};

i32 mtr_execute(struct mtr_vm* vm, struct mtr_package* package);

#endif
