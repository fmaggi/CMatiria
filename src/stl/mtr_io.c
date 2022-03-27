#include "core/log.h"
#include "mtr_stdlib.h"

#include "package.h"
#include "runtime/engine.h"
#include "runtime/object.h"
#include "runtime/value.h"
#include <stdio.h>

#include "debug/disassemble.h"

#include "core/types.h"

mtr_value mtr_print(u8 argc, mtr_value* argv) {
    mtr_value value = *argv;
    switch (value.type) {
    case MTR_VAL_INT: {
        MTR_LOG("%li", value.integer);
        break;
    }
    case MTR_VAL_FLOAT: {
        MTR_LOG("%f", value.floating);
        break;
    }
    case MTR_VAL_OBJ: {
        if (value.object->type != MTR_OBJ_STRING) {
            MTR_LOG_ERROR("Object of type %s is not printable.", mtr_obj_type_to_str(value.object));
            exit(-1);
        }
        struct mtr_string* s = (struct mtr_string*) value.object;
        MTR_LOG("%.*s", (u32)s->length, s->s);
        break;
    }
    }
    return MTR_NIL;
}

void mtr_add_io(struct mtr_package* package) {
    mtr_package_insert_function_by_name(package, (struct mtr_object*)mtr_new_native_function(mtr_print), "print");
}
