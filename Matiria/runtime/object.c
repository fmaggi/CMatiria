#include "object.h"

#include "bytecode.h"
#include "core/log.h"
#include "core/utils.h"

#include <stdlib.h>
#include <string.h>

void mtr_delete_object(struct mtr_object* object) {
    switch (object->type) {
    case MTR_OBJ_STRUCT: {
        struct mtr_struct* s = (struct mtr_struct*) object;
        free(s->members);
        free(s);
        break;
    }
    case MTR_OBJ_STRING: {
        struct mtr_string* s = (struct mtr_string*) object;
        free(s->s);
        free(s);
        break;
    }
    case MTR_OBJ_ARRAY: {
        struct mtr_array* a = (struct mtr_array*) object;
        free(a->elements);
        free(a);
        break;
    }
    case MTR_OBJ_MAP: {
        struct mtr_map* m = (struct mtr_map*) object;
        free(m->entries);
        free(m);
        break;
    }
    case MTR_OBJ_FUNCTION: {
        struct mtr_function* f = (struct mtr_function*) object;
        mtr_delete_chunk(&f->chunk);
        free(f);
        break;
    }
    case MTR_OBJ_NATIVE_FN: {
        struct mtr_native_fn* fn = (struct mtr_native_fn*) object;
        free(fn);
        break;
    }
    case MTR_OBJ_CLOSURE: {
        struct mtr_closure* c = (struct mtr_closure*) object;
        mtr_delete_chunk(&c->chunk);
        free(c->upvalues);
        free(c);
    }
    default:
        break;
    }
}

// Struct

struct mtr_struct* mtr_new_struct(u8 count) {
    struct mtr_struct* s = malloc(sizeof(*s));
    s->obj.type = MTR_OBJ_STRUCT;
    s->members = malloc(sizeof(mtr_value) * count);
    return s;
}

// Struct end

// Function

struct mtr_native_fn* mtr_new_native_function(mtr_native native) {
    struct mtr_native_fn* fn = malloc(sizeof(*fn));
    fn->obj.type = MTR_OBJ_NATIVE_FN;
    fn->function = native;
    return fn;
}

struct mtr_function* mtr_new_function(struct mtr_chunk chunk) {
    struct mtr_function* fn = malloc(sizeof(*fn));
    fn->obj.type = MTR_OBJ_FUNCTION;
    fn->chunk = chunk;
    return fn;
}

// Function End

struct mtr_closure* mtr_new_closure(struct mtr_chunk chunk, mtr_value* upvalues, u8 count) {
    struct mtr_closure* cl = malloc(sizeof(*cl));
    cl->obj.type = MTR_OBJ_CLOSURE;
    cl->chunk = chunk;
    cl->count = count;
    if (upvalues != NULL && count > 0) {
        cl->upvalues = malloc(sizeof(mtr_value) * count);
        memcpy(cl->upvalues, upvalues, sizeof(mtr_value) * count);
    }
    return cl;
}

// Array

struct mtr_array* mtr_new_array(size_t length) {
    struct mtr_array* a = malloc(sizeof(*a));

    a->obj.type = MTR_OBJ_ARRAY;
    a->elements = malloc(sizeof(mtr_value) * length);
    a->capacity = length;
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

// Array end

// String

struct mtr_string* mtr_new_string(const char* string, size_t length) {
    struct mtr_string* s = malloc(sizeof(*s));
    s->obj.type = MTR_OBJ_STRING;

    s->s = malloc(sizeof(char) * length);
    memcpy(s->s, string, sizeof(char) * length);
    s->length = length;
    return s;
}

// String end

// Map

#define LOAD_FACTOR 0.75

struct map_entry {
    mtr_value key;
    mtr_value value;
    bool is_tombstone;
    bool is_used;
};

struct mtr_map_element* mtr_get_key_value_pair(struct mtr_map* map, size_t index) {
    struct map_entry* entry = map->entries + index;
    return entry->is_used ? (struct mtr_map_element*) entry : NULL;
}

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

static u32 hash_val(mtr_value key) {
    if (key.type == MTR_VAL_OBJ) {
        struct mtr_object* obj = key.object;
        if (obj->type != MTR_OBJ_STRING) {
            MTR_LOG_ERROR("Object is not hashable.");
            exit(-1);
        }
        struct mtr_string* s = (struct mtr_string*) obj;
        return hash(s->s, s->length);
    }
    return hashi64(key.integer);
}

static bool compare_keys(mtr_value entry_key, mtr_value key) {
    if (entry_key.type == MTR_VAL_OBJ && key.type == MTR_VAL_OBJ) {
        struct mtr_object* entry_obj = entry_key.object;
        struct mtr_object* obj = key.object;
        if (entry_obj->type != MTR_OBJ_STRING || obj->type != MTR_OBJ_STRING) {
            MTR_LOG_ERROR("Object is not hashable.");
            exit(-1);
        }
        struct mtr_string* entry_s = (struct mtr_string*) entry_obj;
        struct mtr_string* s = (struct mtr_string*) obj;
        return entry_s->length == s->length && memcmp(entry_s->s, s->s, s->length) == 0;;
    }
    return entry_key.integer == key.integer;
}

static struct map_entry* find_entry(struct map_entry* entries, mtr_value key, size_t cap, bool return_tombstone) {
    u32 hash_ = hash_val(key);
    u32 index = hash_ & (cap - 1);

    struct map_entry* entry = entries + index;
    while (entry->is_used && !(return_tombstone && entry->is_tombstone)) {
        bool same = compare_keys(entry->key, key);
        if (same) {
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

// Map end
