#ifndef MTR_OBJECT_H
#define MTR_OBJECT_H

#include "value.h"
#include "bytecode.h"

#include "core/types.h"

enum mtr_object_t {
    MTR_OBJ_STRUCT,
    MTR_OBJ_FUNCTION,
    MTR_OBJ_NATIVE_FN,
    MTR_OBJ_STRING,
    MTR_OBJ_ARRAY,
    MTR_OBJ_MAP
};

struct mtr_object {
    enum mtr_object_t type;
};

void mtr_delete_object(struct mtr_object* object);

struct mtr_engine;

struct mtr_struct {
    struct mtr_object obj;
    mtr_value* members;
};

struct mtr_struct* mtr_new_struct(u8 count);

typedef mtr_value (*mtr_native)(u8 argc, mtr_value* first);

struct mtr_native_fn {
    struct mtr_object obj;
    mtr_native function;
};

struct mtr_native_fn* mtr_new_native_function(mtr_native native);

struct mtr_function {
    struct mtr_object obj;
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

struct mtr_string {
    struct mtr_object obj;
    char* s;
    size_t length;
};

struct mtr_string* mtr_new_string(const char* string, size_t length);

struct mtr_map {
    struct mtr_object obj;
    struct map_entry* entries;
    size_t size;
    size_t capacity;
};

struct mtr_map* mtr_new_map(void);
void mtr_delete_map(struct mtr_map* map);

void mtr_map_insert(struct mtr_map* map, mtr_value key, mtr_value value);
mtr_value mtr_map_get(struct mtr_map* map, mtr_value key);
mtr_value mtr_map_remove(struct mtr_map* map, mtr_value key);

#endif
