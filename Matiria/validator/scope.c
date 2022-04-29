#include "scope.h"

#include "core/log.h"
#include "core/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOAD_FACTOR 0.75

static const char* tombstone = "__@tombstone@__@mangled@__";

struct symbol_entry {
    struct mtr_symbol symbol;
    const char* key;
    size_t length;
};

struct mtr_symbol_table mtr_new_symbol_table(void) {
    struct mtr_symbol_table t = {
        .entries = NULL,
        .capacity = 0,
        .size = 0,
    };

    t.capacity = 8;
    t.entries = calloc(8, sizeof(struct symbol_entry));

    return t;
}

void mtr_delete_symbol_table(struct mtr_symbol_table* table) {
    free(table->entries);
    table->capacity = 0;
    table->size = 0;
    table->entries = NULL;
}

static struct symbol_entry* find_entry(struct symbol_entry* entries, const char* key, size_t length, size_t cap, bool return_tombstone) {
    u32 hash_ = hash(key, length);
    u32 index = hash_ & (cap - 1);

    struct symbol_entry* entry = entries + index;

    while (entry->key != NULL && !(return_tombstone && (entry->key == tombstone))) {
        if (entry->length == length && hash(entry->key, entry->length) == hash_ && memcmp(key, entry->key, entry->length) == 0)
            break;

        index = (index + 1) & (cap - 1);
        entry = entries + index;
    }

    return entry;
}

static struct symbol_entry* resize_entries(struct symbol_entry* entries, size_t old_cap) {
    size_t new_cap = old_cap * 2;
    struct symbol_entry* temp = calloc(new_cap, sizeof(struct symbol_entry));

    for (size_t i = 0; i < old_cap; ++i) {
        struct symbol_entry* old = entries + i;
        if (old->key == NULL || old->key == tombstone)
            continue;
        struct symbol_entry* entry = find_entry(temp, old->key, old->length, new_cap, true);
        entry->key = old->key;
        entry->symbol = old->symbol;
        entry->length = old->length;
    }
    free(entries);
    return temp;
}

void mtr_symbol_table_insert(struct mtr_symbol_table* table, const char* key, size_t length, struct mtr_symbol symbol) {
    struct symbol_entry* entry = find_entry(table->entries, key, length, table->capacity, true);
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
    struct symbol_entry* entry = find_entry(table->entries, key, length, table->capacity, false);
    if (NULL == entry || NULL == entry->key) {
        return NULL;
    }
    MTR_ASSERT(entry->key != tombstone, "key should not be tombstone at get");
    return &entry->symbol;
}


void mtr_symbol_table_remove(const struct mtr_symbol_table* table, const char* key, size_t length) {
    struct symbol_entry* entry = find_entry(table->entries, key, length, table->capacity, false);
    if (NULL == entry || NULL == entry->key) {
        return;
    }
    MTR_ASSERT(entry->key != tombstone, "key should not be tombstone at delete");
    entry->key = tombstone;
    entry->length = strlen(tombstone);
}

struct mtr_scope mtr_new_scope(struct mtr_scope* parent) {
    struct mtr_scope scope;
    scope.parent = parent;
    scope.symbols = mtr_new_symbol_table();
    bool should_be_zero = parent == NULL || parent->parent == NULL;
    scope.current = should_be_zero ? 0 : parent->current;
    return scope;
}

void mtr_delete_scope(struct mtr_scope* scope) {
    mtr_delete_symbol_table(&scope->symbols);
}

struct mtr_symbol* mtr_scope_find(const struct mtr_scope* scope, struct mtr_token token) {
    struct mtr_symbol* s = mtr_symbol_table_get(&scope->symbols, token.start, token.length);
    while (NULL == s && NULL != scope->parent) {
        scope = scope->parent;
        s = mtr_symbol_table_get(&scope->symbols, token.start, token.length);
    }
    return s;
}

struct mtr_symbol* mtr_scope_add(struct mtr_scope* scope, struct mtr_symbol symbol) {
    struct mtr_symbol* s = mtr_scope_find(scope, symbol.token);
    if (NULL != s) {
        return s;
    }

    symbol.index = scope->current++;
    symbol.type.is_global = scope->parent == NULL;
    mtr_symbol_table_insert(&scope->symbols, symbol.token.start, symbol.token.length, symbol);
    return NULL;
}
