#include "object.h"

#include "AST/expr.h"
#include "engine.h"

#include "core/log.h"

#include <stdlib.h>

void mtr_delete_object(struct mtr_object* object) {
    IMPLEMENT
}

static void native_fn_call(struct mtr_object* function, struct mtr_engine* engine, u8 argc) {
    struct mtr_native_fn* n = (struct mtr_native_fn*) function;
    n->function(engine, argc, engine->stack_top - argc);
    engine->stack_top -= argc;
}

static void function_call(struct mtr_object* function, struct mtr_engine* engine, u8 argc) {
    struct mtr_function* f = (struct mtr_function*) function;
    mtr_call(engine, f->chunk, argc);
}

struct mtr_native_fn* mtr_new_native_function(mtr_native native) {
    struct mtr_native_fn* fn = malloc(sizeof(*fn));
    fn->method.call = native_fn_call;
    fn->method.obj.type = MTR_OBJ_NATIVE_FN;
    fn->function = native;
    return fn;
}

struct mtr_function* mtr_new_function(struct mtr_chunk chunk) {
    struct mtr_function* fn = malloc(sizeof(*fn));
    fn->method.call = function_call;
    fn->method.obj.type = MTR_OBJ_FUNCTION;
    fn->chunk = chunk;
    return fn;
}

struct mtr_array* mtr_new_array(void) {
    struct mtr_array* a = malloc(sizeof(*a));
    if (NULL == a) {
        MTR_LOG_ERROR("Bad allocation.");
        return NULL;
    }

    void* temp = malloc(sizeof(mtr_value) * 8);
    if (NULL == temp) {
        free(a);
        MTR_LOG_ERROR("Bad allocation.");
        return NULL;
    }

    a->obj.type = MTR_OBJ_ARRAY;
    a->elements = temp;
    a->capacity = 8;
    a->size = 0;

    for (u8 i = 0; i < 8; ++i) {
        a->elements[i] = (mtr_value){ .integer = i } ;
    }

    return a;
}

void mtr_delete_array(struct mtr_array* array) {
    free(array->elements);
    array->elements = NULL;
    free(array);
}

void mtr_array_append(struct mtr_array* array, mtr_value value) {
    if (array->size == array->capacity) {
        size_t new_cap = array->capacity * 2;
        array->elements = realloc(array->elements, new_cap * sizeof(mtr_value));
        array->capacity = new_cap;
    }

    array->elements[array->size++] = value;
}

mtr_value mtr_array_pop(struct mtr_array* array) {
    return array->elements[--array->size];
}
