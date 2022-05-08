#include "typeList.h"
#include "AST/type.h"
#include "core/log.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct type_entry {
    struct mtr_type* type;
    size_t hash;
    bool used;
};

static size_t hash_type(struct mtr_type* type) {
    if (type == NULL) {
        return 0;
    }
    switch (type->type) {
    case MTR_DATA_INVALID:
    case MTR_DATA_ANY:
    case MTR_DATA_VOID:
    case MTR_DATA_BOOL:
    case MTR_DATA_INT:
    case MTR_DATA_FLOAT:
    case MTR_DATA_STRING: {
        return (size_t) type->type;
    }

    case MTR_DATA_ARRAY: {
        struct mtr_array_type* a = (struct mtr_array_type*) type;
        return MTR_DATA_ARRAY
                ^ (hash_type(a->element) << 1) * 21;
    }
    case MTR_DATA_MAP: {
        struct mtr_map_type* m = (struct mtr_map_type*) type;
        return ((MTR_DATA_MAP
                ^ (hash_type(m->key) << 5)) >> 8)
                ^ (hash_type(m->value) << 13) * 21;
    }
    case MTR_DATA_FN: {
        struct mtr_function_type* f = (struct mtr_function_type*) type;
        size_t h = MTR_DATA_FN ^ (hash_type(f->return_) << 7);
        for (u8 i = 0; i < f->argc; ++i) {
            h ^= (hash_type(f->argv[i]) << i * 17);
            h = h * 21;
        }
        h += h << 11;
        return h;
    }
    case MTR_DATA_USER:
    case MTR_DATA_UNION:
    case MTR_DATA_STRUCT: {
        IMPLEMENT;
        return 0;
    }
    }
}

// this is basically hashing
static struct type_entry* find_entry(struct mtr_type* type, struct type_entry* entries, size_t capacity) {
    size_t t = (size_t) type->type;
    if (!mtr_is_compound_type(type)) {
        return entries + t; // there is always room for basic types in here so this will always fit
    }

    u8 basic_types_count = 7;

    size_t h = hash_type(type);
    size_t i = h & (capacity - 1);

#ifndef NDEBUG
    size_t iters = 0;
#endif

    struct type_entry e = entries[i];
    while (e.used == true) {
        if (h == entries[i].hash && mtr_type_match(type, e.type)) {
            return entries + i;
        }
#ifndef NDEBUG
        iters++;
#endif
        i = (i + 1) & (capacity - 1);
        e = entries[i];
    }

// #ifndef NDEBUG
//     MTR_LOG_WARN("%zu collisions with size %zu", iters, capacity);
// #endif
    MTR_ASSERT(i >= 7, "Whaaaaaaaaaaaat");
    return entries + i;
}

static struct mtr_type* insert(struct mtr_type_list* list, void* type, size_t size_type);
#define INSERT(type) insert(list, type, sizeof(*type))

void mtr_type_list_init(struct mtr_type_list* list) {
    list->types = calloc(16, sizeof(struct type_entry));
    list->capacity = 16;
    list->count = 0;

    for (u8 i = 0; i < MTR_DATA_STRING + 1; ++i) {
        struct mtr_type* t = malloc(sizeof(*t));
        t->type = i;
        list->types[i].type = t;
        list->types[i].hash = i;
        list->types[i].used = true;
        list->count++;
    }
}

void mtr_type_list_delete(struct mtr_type_list* list) {
    free(list->types);
    list->capacity = 0;
    list->count = 0;
    list->types = NULL;
}

#define LOAD_FACTOR 0.75

static struct type_entry* resize(struct type_entry* entries, size_t old_cap) {
    MTR_PROFILE_FUNC();
    size_t new_cap = old_cap * 2;
    struct type_entry* temp = calloc(new_cap, sizeof(struct type_entry));

    for (size_t i = 0; i < old_cap; ++i) {
        struct type_entry old = entries[i];
        if (!old.used)
            continue;
        struct type_entry* entry = find_entry(old.type, temp, new_cap);
        entry->type = old.type;
        entry->hash = old.hash;
        entry->used = true;
        MTR_LOG_WARN("moving %p", (void*) old.type);
    }
    free(entries);
    return temp;

}

static struct mtr_type* insert(struct mtr_type_list* list, void* type, size_t size_type) {
    struct mtr_type* actual_type = type;
    MTR_ASSERT(list->count < list->capacity, "Wrong size!");
    struct type_entry* entry = find_entry(actual_type, list->types, list->capacity);

    if (entry->type && mtr_type_match(actual_type, entry->type)) {
        return entry->type;
    }

    entry->type = malloc(size_type);
    memcpy(entry->type, type, size_type);
    entry->hash = hash_type(type);
    entry->used = true;
    list->count++;

    if (list->count >= list->capacity * LOAD_FACTOR) {
        list->types = resize(list->types, list->capacity);
        list->capacity *= 2;
    }
    return entry->type;
}

struct mtr_type* mtr_type_list_register_from_token(struct mtr_type_list* list, struct mtr_token token) {
    struct mtr_type type = mtr_get_data_type(token);
    return INSERT(&type);
}


struct mtr_type* mtr_type_list_register_array(struct mtr_type_list* list, struct mtr_type* element) {
    struct mtr_array_type a;
    a.type.type = MTR_DATA_ARRAY;
    a.element = element;

    return INSERT(&a);
}

struct mtr_type* mtr_type_list_register_map(struct mtr_type_list* list, struct mtr_type* key, struct mtr_type* value) {
    struct mtr_map_type m;
    m.type.type = MTR_DATA_MAP;
    m.key = key;
    m.value = value;

    return INSERT(&m);
}

struct mtr_type* mtr_type_list_register_function(struct mtr_type_list* list, struct mtr_type* ret, struct mtr_type** argv, u8 argc) {
    struct mtr_function_type f;
    f.type.type = MTR_DATA_FN;
    f.argc = argc;
    f.argv = argv;
    f.return_ = ret;

    struct mtr_function_type* r = (void*) INSERT(&f);

    if (r->argv == argv) {
        if (argc == 0) {
            r->argv = NULL;
            return (void*)r;
        }
        void* temp = malloc(sizeof(struct mtr_type*) * argc);
        memcpy(temp, argv, sizeof(struct mtr_type*) * argc);
        r->argv = temp;
    }


    return (void*)r;
}

struct mtr_type* mtr_type_list_get(struct mtr_type_list* list, size_t index) {
    return list->types[index].type;
}

struct mtr_type* mtr_type_list_get_user_type(struct mtr_type_list* list, struct mtr_token token) {
    IMPLEMENT
    return NULL;
}

struct mtr_type* mtr_type_list_get_void_type(struct mtr_type_list* list) {
    return list->types[MTR_DATA_VOID].type;
}

struct mtr_type* mtr_type_list_exists(struct mtr_type_list* list, struct mtr_type type) {
    struct type_entry* entry = find_entry(&type, list->types, list->count);
    if (entry->type && mtr_type_match(&type, entry->type)) {
        return entry->type;
    }
    return NULL;
}
