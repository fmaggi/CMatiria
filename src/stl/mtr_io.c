#include "mtr_stdlib.h"

#include "runtime/engine.h"
#include "runtime/object.h"
#include "runtime/value.h"
#include <stdio.h>

#include "core/types.h"

void mtr_print(struct mtr_engine* engine, u8 argc, mtr_value* argv) {
    for (u8 i = 0; i < argc; ++i) {
        mtr_value value = argv[i];
        printf("%li\n", value.integer);
    }
}

void mtr_add_io(struct mtr_package* package) {
    struct mtr_native_fn* f = mtr_new_native_function(mtr_print);
    mtr_package_insert_function_by_name(package, (struct mtr_object*)f, "print");
}
