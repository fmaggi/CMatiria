#ifndef MTR_OBJECT_H
#define MTR_OBJECT_H

#include "validator/type.h"
#include "value.h"
#include "bytecode.h"

#include "core/types.h"

enum mtr_object_t {
    MTR_OBJ_FUNCTION,
    MTR_OBJ_NATIVE_FN,
    MTR_OBJ_ARRAY
};

struct mtr_object {
    enum mtr_object_t type;
};

void mtr_delete_object(struct mtr_object* object);

struct mtr_engine;

typedef void (*mtr_invoke)(struct mtr_object* function, struct mtr_engine* engine, u8 argc);

struct mtr_invokable {
    struct mtr_object obj;
    mtr_invoke call;
};

typedef mtr_value (*mtr_native)(u8 argc, mtr_value* first);

struct mtr_native_fn {
    struct mtr_invokable method;
    mtr_native function;
};

struct mtr_native_fn* mtr_new_native_function(mtr_native native);

struct mtr_function {
    struct mtr_invokable method;
    struct mtr_chunk chunk;
};

struct mtr_function* mtr_new_function(struct mtr_chunk chunk);

struct mtr_array {
    struct mtr_object obj;
    mtr_value* elements;
    size_t size;
    size_t capacity;
};

struct mtr_array* mtr_new_array(void);
void mtr_delete_array(struct mtr_array* array);

void mtr_array_append(struct mtr_array* array, mtr_value value);
mtr_value mtr_array_pop(struct mtr_array* array);
// void mtr_array_insert(struct mtr_array* array, mtr_value value, size_t index);

#endif