#include "object.h"

#include "AST/expr.h"
#include "engine.h"

#include "core/log.h"
#include "runtime/value.h"

#include <stdlib.h>

void mtr_delete_object(struct mtr_object* object) {
    IMPLEMENT
}

static void native_fn_call(struct mtr_object* function, struct mtr_engine* engine, u8 argc) {
    struct mtr_native_fn* n = (struct mtr_native_fn*) function;
    mtr_value val = n->function(argc, engine->stack_top - argc);
    engine->stack_top -= argc;
    mtr_push(engine, val);
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

    a->obj.type = MTR_OBJ_ARRAY;
    a->elements = malloc(sizeof(mtr_value) * 8);
    a->capacity = 8;
    a->size = 0;

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

#define LOAD_FACTOR 0.75

struct map_entry {
    mtr_value key;
    mtr_value value;
    bool is_tombstone;
    bool is_used;
};

struct mtr_map* mtr_new_map(void) {
    struct mtr_map* map = malloc(sizeof(*map));

    map->obj.type = MTR_OBJ_MAP;
    map->entries = calloc(8, sizeof(struct map_entry));
    map->capacity = 8;
    map->size = 0;

    return map;
}

void mtr_delete_map(struct mtr_map* map) {
    free(map->entries);
    map->entries = NULL;
    map->capacity = 0;
    map->size = 0;
    free(map);
}

// got it from http://web.archive.org/web/20071223173210/http:/www.concentric.net/~Ttwang/tech/inthash.htm
static u32 hashi64(i64 key) {
    key = (~key) + (key << 18);
    key = key ^ (key >> 31);
    key = key * 21;
    key = key ^ (key >> 11);
    key = key + (key << 6);
    key = key ^ (key >> 22);
    return (u32) key;
}

static struct map_entry* find_entry(struct map_entry* entries, mtr_value key, size_t cap, bool return_tombstone) {
    u32 hash_ = hashi64(key.integer);
    u32 index = hash_ & (cap - 1);

    struct map_entry* entry = entries + index;
    while (entry->is_used && !(return_tombstone && entry->is_tombstone)) {
        if (entry->key.integer == key.integer) {
            break;
        }

        index = (index + 1) & (cap - 1);
        entry = entries + index;
    }

    return entry;
}

static struct map_entry* resize_entries(struct map_entry* entries, size_t old_cap) {
    size_t new_cap = old_cap * 2;
    struct map_entry* temp = calloc(new_cap, sizeof(struct map_entry));

    for (size_t i = 0; i < old_cap; ++i) {
        struct map_entry* old = entries + i;
        if (!old->is_used || old->is_tombstone)
            continue;
        struct map_entry* entry = find_entry(temp, old->key, new_cap, true);
        entry->key = old->key;
        entry->value = old->value;
        entry->is_used = true;
        entry->is_tombstone = false;
    }

    free(entries);
    return temp;
}

void mtr_map_insert(struct mtr_map* map, mtr_value key, mtr_value value) {
    struct map_entry* entry = find_entry(map->entries, key, map->capacity, true);
    entry->value = value;

    if (entry->is_used && !entry->is_tombstone) {
        return;
    }

    entry->key = key;
    entry->is_used = true;

    if (entry->is_tombstone) {
        return;
    }

    map->size += 1;
    if (map->size >= map->capacity * LOAD_FACTOR) {
        map->entries = resize_entries(map->entries, map->capacity);
        map->capacity *= 2;
    }
}

mtr_value mtr_map_get(struct mtr_map* map, mtr_value key) {
    struct map_entry* entry = find_entry(map->entries, key, map->capacity, false);
    if (!entry->is_used) {
        return MTR_NIL;
    }

    return entry->value;
}


mtr_value mtr_map_remove(struct mtr_map* map, mtr_value key) {
    struct map_entry* entry = find_entry(map->entries, key, map->capacity, false);
    if (!entry->is_used) {
        return MTR_NIL;
    }
    entry->is_tombstone = true;
    return entry->value;
}
