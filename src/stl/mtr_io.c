#include "core/log.h"
#include "mtr_stdlib.h"

#include "package.h"
#include "runtime/engine.h"
#include "runtime/object.h"
#include "runtime/value.h"
#include <stdio.h>

#include "debug/disassemble.h"

#include "core/types.h"

static void print_value(mtr_value value) {
    switch (value.type) {
    case MTR_VAL_INT: {
        MTR_PRINT("%li", value.integer);
        break;
    }
    case MTR_VAL_FLOAT: {
        MTR_PRINT("%f", value.floating);
        break;
    }
    case MTR_VAL_OBJ: {
        switch (value.object->type) {
        case MTR_OBJ_STRING: {
            struct mtr_string* s = (struct mtr_string*) value.object;
            MTR_PRINT("'%.*s'", (u32)s->length, s->s);
            break;
        }
        case MTR_OBJ_ARRAY: {
            struct mtr_array* a = (struct mtr_array*) value.object;
            if (a->size == 0) {
                MTR_PRINT("[]");
                break;
            }
            MTR_PRINT("[");
            for (size_t i = 0; i < a->size-1; ++i) {
                print_value(a->elements[i]);
                MTR_PRINT(", ");
            }
            print_value(a->elements[a->size-1]);
            MTR_PRINT("]");
            break;
        }
        case MTR_OBJ_MAP: {
            struct mtr_map* m = (struct mtr_map*) value.object;
            MTR_PRINT("{");

            size_t i = 0;
            struct mtr_map_element* e = NULL;

            // print first element
            for (; i < m->capacity-1; ++i) {
                e = mtr_get_key_value_pair(m, i);
                if (e == NULL) {
                    continue;
                }

                print_value(e->key);
                MTR_PRINT(": ");
                print_value(e->value);
                break;
            }

            // print middle elements;
            for (++i; i < m->capacity-1; ++i) {
                e = mtr_get_key_value_pair(m, i);
                if (e == NULL) {
                    continue;
                }
                MTR_PRINT(", ");
                print_value(e->key);
                MTR_PRINT(": ");
                print_value(e->value);
            }

            // print last elements;
            e = mtr_get_key_value_pair(m, m->capacity-1);
            if (e != NULL) {
                MTR_PRINT(", ");
                print_value(e->key);
                MTR_PRINT(": ");
                print_value(e->value);
            }
            MTR_PRINT("}");
            break;
        }
        case MTR_OBJ_FUNCTION:
        case MTR_OBJ_NATIVE_FN:
            MTR_PRINT("%s", mtr_obj_type_to_str(value.object));
        case MTR_OBJ_STRUCT:
            MTR_PRINT("%s is not printable", mtr_obj_type_to_str(value.object));
        }
    }
    }
}

mtr_value mtr_print(u8 argc, mtr_value* argv) {
    mtr_value value = *argv;
    print_value(value);
    MTR_PRINT("\n");
    return MTR_NIL;
}

void mtr_add_io(struct mtr_package* package) {
    struct mtr_native_fn* n = mtr_new_native_function(mtr_print);
    mtr_package_insert_native_function(package, (struct mtr_object*)n, "print");
}
