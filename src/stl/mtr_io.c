#include "mtr_stdlib.h"

#include "package.h"
#include "runtime/engine.h"
#include "runtime/object.h"
#include "runtime/value.h"
#include <stdio.h>

#include "core/types.h"

mtr_value mtr_print(u8 argc, mtr_value* argv) {
    for (u8 i = 0; i < argc; ++i) {
        mtr_value value = argv[i];
        printf("%li\n", value.integer);
    }
    return (mtr_value)MTR_NIL;
}

void mtr_add_io(struct mtr_package* package) {
    mtr_package_insert_function_by_name(package, (struct mtr_object*)mtr_new_native_function(mtr_print), "print");
}
