#include "typeList.h"
#include "AST/type.h"
#include "core/log.h"
#include "core/utils.h"
#include "scanner/token.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// this should be const
static struct mtr_type invalid_type = {.type = MTR_DATA_INVALID};
static struct mtr_type any_type = { .type = MTR_DATA_ANY };
static struct mtr_type void_type = {.type = MTR_DATA_VOID };
static struct mtr_type bool_type = { .type = MTR_DATA_BOOL };
static struct mtr_type int_type = { .type = MTR_DATA_INT };
static struct mtr_type float_type = { .type = MTR_DATA_FLOAT };
static struct mtr_type string_type = { .type = MTR_DATA_STRING };

struct type_entry {
    struct mtr_type* type;
    size_t hash;
};

void mtr_type_list_init(struct mtr_type_list* list) {
    list->types = calloc(16, sizeof(struct type_entry));
    list->capacity = 16;
    list->count = 7;

#define LOAD_TYPE(ptr, enume) \
    list->types[enume] = (struct type_entry){ .type = ptr, .hash = enume }

    LOAD_TYPE(&invalid_type, MTR_DATA_INVALID);
    LOAD_TYPE(&any_type, MTR_DATA_ANY);
    LOAD_TYPE(&void_type, MTR_DATA_VOID);
    LOAD_TYPE(&bool_type, MTR_DATA_BOOL);
    LOAD_TYPE(&int_type, MTR_DATA_INT);
    LOAD_TYPE(&float_type, MTR_DATA_FLOAT);
    LOAD_TYPE(&string_type, MTR_DATA_STRING);

#undef LOAD_TYPE
}

void mtr_type_list_delete(struct mtr_type_list* list) {
    for (size_t i = 7; i < list->capacity; ++i) {
        if (!list->types[i].type) {
            continue;
        }
        mtr_delete_type(list->types[i].type);
        free(list->types[i].type);
    }

    free(list->types);
    list->capacity = 0;
    list->count = 0;
    list->types = NULL;
}

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
        struct mtr_user_type* u = (struct mtr_user_type*) type;
        return hash(u->name.start, u->name.length);
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
    while (e.type != NULL) {
        if (h == e.hash && mtr_type_match(type, e.type)) {
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

#define LOAD_FACTOR 0.75

static struct type_entry* resize(struct type_entry* entries, size_t old_cap) {
    size_t new_cap = old_cap * 2;
    struct type_entry* temp = calloc(new_cap, sizeof(struct type_entry));

    for (size_t i = 0; i < old_cap; ++i) {
        struct type_entry old = entries[i];
        if (!old.type)
            continue;
        struct type_entry* entry = find_entry(old.type, temp, new_cap);
        entry->type = old.type;
        entry->hash = old.hash;
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

struct mtr_type* mtr_type_list_register_struct_type(struct mtr_type_list* list, struct mtr_token name, struct mtr_symbol** members, u16 count) {
    struct mtr_struct_type s;
    s.name.type.type = MTR_DATA_STRUCT;
    s.name.name = name;
    s.members = members;
    s.argc = count;

    struct mtr_struct_type* r = (void*) INSERT(&s);

    if (r->members == members) {
        if (count == 0) {
            r->members = NULL;
            return (void*) r;
        }
        void* temp = malloc(sizeof(struct mtr_symbol*) * count);
        memcpy(temp, members, sizeof(struct mtr_symbol*) * count);
        r->members = temp;
    }
    return (void*)r;
}

struct mtr_type* mtr_type_list_register_union_type(struct mtr_type_list* list, struct mtr_token name, struct mtr_type** types, u16 count) {
    struct mtr_union_type u;
    u.name.type.type = MTR_DATA_UNION;
    u.name.name = name;
    u.types = types;
    u.argc = count;

    struct mtr_union_type* r = (void*) INSERT(&u);

    if (r->types == types) {
        if (count == 0) {
            r->types = NULL;
            return (void*) r;
        }
        void* temp = malloc(sizeof(struct mtr_type*) * count);
        memcpy(temp, types, sizeof(struct mtr_type*) * count);
        r->types = temp;
    }
    return (void*)r;
}

struct mtr_type* mtr_type_list_get(struct mtr_type_list* list, size_t index) {
    return list->types[index].type;
}

static bool is_user(struct mtr_type* type) {
    return type->type == MTR_DATA_USER || type->type == MTR_DATA_STRUCT || type->type == MTR_DATA_UNION;
}

struct mtr_type* mtr_type_list_get_user_type(struct mtr_type_list* list, struct mtr_token token) {
    size_t h = hash(token.start, token.length);
    size_t i = h & (list->capacity - 1);

    struct type_entry e = list->types[i];
    while (e.type != NULL) {
        struct mtr_user_type* u = (struct mtr_user_type*) e.type;
        if (h == e.hash && is_user(e.type) && mtr_token_compare(u->name, token)) {
            return e.type;
        }

        i = (i + 1) & (list->capacity - 1);
        e = list->types[i];
    }

    return e.type;
}

struct mtr_type* mtr_type_list_get_void_type(struct mtr_type_list* list) {
    return list->types[MTR_DATA_VOID].type;
}

struct mtr_type* mtr_type_list_exists(struct mtr_type_list* list, struct mtr_type type) {
    struct type_entry* entry = find_entry(&type, list->types, list->capacity);
    if (entry->type && mtr_type_match(&type, entry->type)) {
        return entry->type;
    }
    return NULL;
}
