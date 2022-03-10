#include "bytecode.h"
#include "mtr_stdlib.h"

#include "core/types.h"

#include "runtime/object.h"
#include "runtime/value.h"
#include "validator/type.h"
#include <stdio.h>

mtr_value mtr_print(u8 argc, mtr_value* argv) {
    for (u8 i = 0; i < argc; ++i) {
        mtr_value value = argv[i];
        printf("%li\n", value.integer);
    }
    mtr_value ret = MTR_NIL;
    return ret;
}

void mtr_add_io(struct mtr_package* package) {
    struct mtr_native_fn* f = mtr_new_native_function(mtr_print);
    mtr_package_insert_function_by_name(package, (struct mtr_object*)f, "print");
}
