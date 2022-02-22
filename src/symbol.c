#include "symbol.h"

#include "core/log.h"

#include <stdlib.h>
#include <string.h>

#define LOAD_FACTOR 0.75

static const char* tombstone = "__@tombstone@__@mangled@__";

struct mtr_symbol_table mtr_new_symbol_table() {
    struct mtr_symbol_table t = {
        .entries = NULL,
        .capacity = 0,
        .size = 0
    };

    void* temp = calloc(8, sizeof(struct mtr_entry));

    if (NULL != temp) {
        t.capacity = 8;
        t.entries = temp;
    } else {
        MTR_LOG_ERROR("Bad allocation.");
    }

    return t;
}

void mtr_delete_symbol_table(struct mtr_symbol_table* table) {
    table->entries = 0;
    table->size = 0;
    free(table->entries);
    table->entries = NULL;
}

static u32 hash(const char* key, size_t length) {
    u32 hash = 2166136261u;
    for (size_t i = 0; i < length; i++) {
        hash ^= (u8)key[i];
        hash *= 16777619;
    }
    return hash;
}

static struct mtr_entry* find_entry(struct mtr_entry* entries, const char* key, size_t length, size_t cap, bool return_tombstone) {
    u32 hash_ = hash(key, length);
    u32 start_index = hash_ % cap;

    u32 index = start_index;
    struct mtr_entry* entry = entries + index;

    while (entry->key != NULL && !((entry->key == tombstone) && return_tombstone)) {
        size_t s = entry->length;
        if (s == length && hash(entry->key, s) == hash_ && memcmp(key, entry->key, s) == 0)
            break;

        index = (index + 1) % cap;
        if (index == start_index)
            return NULL;

        entry = entries + index;
    }

    return entry;
}

static struct mtr_entry* resize_entries(struct mtr_entry* entries, size_t old_cap) {
    size_t new_cap = old_cap * 2;
    struct mtr_entry* temp = calloc(new_cap, sizeof(struct mtr_entry));

    if (NULL == temp) {
        MTR_LOG_ERROR("Bad allocation.");
        free(temp);
        free(entries);
        return NULL;
    }

    for (size_t i = 0; i < old_cap; ++i) {
        struct mtr_entry* old = entries + i;
        if (old->key == NULL)
            continue;
        struct mtr_entry* entry = find_entry(temp, old->key, old->length, new_cap, true);
        entry->key = old->key;
        entry->symbol = old->symbol;
        entry->length = old->length;
    }
    free(entries);
    return temp;
}

void mtr_symbol_table_insert(struct mtr_symbol_table* table, const char* key, size_t length, struct mtr_symbol symbol) {
    struct mtr_entry* entry = find_entry(table->entries, key, length, table->capacity, true);
    entry->symbol = symbol;
    bool is_tombstone = entry->key == tombstone;

    if (entry->key != NULL && !is_tombstone) {
        return;
    }

    entry->key = key;
    entry->length = length;

    if (is_tombstone) {
        return;
    }

    table->size += 1;
    if (table->size >= table->capacity * LOAD_FACTOR) {
        table->entries = resize_entries(table->entries, table->capacity);
        table->capacity *= 2;
    }
}

struct mtr_symbol* mtr_symbol_table_get(const struct mtr_symbol_table* table, const char* key, size_t length) {
    struct mtr_entry* entry = find_entry(table->entries, key, length, table->capacity, false);
    if (NULL == entry || NULL == entry->key) {
        return NULL;
    }
    MTR_ASSERT(entry->key != tombstone, "key should not be tombstone at get");
    return &entry->symbol;
}


void mtr_symbol_table_remove(const struct mtr_symbol_table* table, const char* key, size_t length) {
    struct mtr_entry* entry = find_entry(table->entries, key, length, table->capacity, false);
    if (NULL == entry || NULL == entry->key) {
        return;
    }
    MTR_ASSERT(entry->key != tombstone, "key should not be tombstone at delete");
    entry->key = tombstone;
    entry->length = strlen(tombstone);
}

enum mtr_data_type_e mtr_get_data_type(enum mtr_token_type type) {
    switch (type)
    {
    case MTR_TOKEN_U8:   return MTR_DATA_U8;
    case MTR_TOKEN_U16:  return MTR_DATA_U16;
    case MTR_TOKEN_U32:  return MTR_DATA_U32;
    case MTR_TOKEN_U64:  return MTR_DATA_U64;
    case MTR_TOKEN_I8:   return MTR_DATA_I8;
    case MTR_TOKEN_I16:  return MTR_DATA_I16;
    case MTR_TOKEN_I32:  return MTR_DATA_I32;
    case MTR_TOKEN_I64:  return MTR_DATA_I64;
    case MTR_TOKEN_F32:  return MTR_DATA_F32;
    case MTR_TOKEN_F64:  return MTR_DATA_F64;
    case MTR_TOKEN_BOOL: return MTR_DATA_BOOL;
    default:
        return MTR_DATA_INVALID;
    }
}

struct mtr_scope mtr_new_scope(struct mtr_scope* parent) {
    struct mtr_scope scope = {
        .parent = parent,
        .symbols = mtr_new_symbol_table()
    };
    return scope;
}

struct mtr_symbol* mtr_scope_find(const struct mtr_scope* scope, const char* key, size_t length) {
    struct mtr_symbol* symbol = mtr_symbol_table_get(&scope->symbols, key, length);
    while (NULL == symbol && NULL != scope->parent) {
        scope = scope->parent;
        symbol = mtr_symbol_table_get(&scope->symbols, key, length);
    }
    return symbol;
}
