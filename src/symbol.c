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

    return temp;
}

void mtr_insert_symbol(struct mtr_symbol_table* table, const char* key, size_t length, struct mtr_symbol symbol) {
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
        if (NULL == table->entries) {
            table->capacity = 0;
            table->size = 0;
        }
    }
}

struct mtr_symbol* mtr_get_symbol(const struct mtr_symbol_table* table, const char* key, size_t length) {
    struct mtr_entry* entry = find_entry(table->entries, key, length, table->capacity, false);
    if (NULL == entry || NULL == entry->key) {
        return NULL;
    }
    MTR_ASSERT(entry->key != tombstone, "key should not be tombstone at get");
    return &entry->symbol;
}


void mtr_delete_symbol(const struct mtr_symbol_table* table, const char* key, size_t length) {
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

static void walk_stmt(struct mtr_stmt* stmt, struct mtr_symbol_table* table);

static void walk_var(struct mtr_var_decl* stmt, struct mtr_symbol_table* table) {
    struct mtr_symbol s;
    s.is_callable = false;
    s.type.type = mtr_get_data_type(stmt->var_type);

    if (NULL != mtr_get_symbol(table, stmt->name.start, stmt->name.length))
        MTR_LOG_ERROR("Redefinition of name");

    mtr_insert_symbol(table, stmt->name.start, stmt->name.length, s);
}

static void walk_fn(struct mtr_fn_decl* stmt, struct mtr_symbol_table* table) {
    struct mtr_symbol s;
    s.is_callable = true;
    s.type.type = mtr_get_data_type(stmt->return_type);

    if (NULL != mtr_get_symbol(table, stmt->name.start, stmt->name.length))
        MTR_LOG_ERROR("Redefinition of name");

    mtr_insert_symbol(table, stmt->name.start, stmt->name.length, s);
}

static void walk_if(struct mtr_if* stmt, struct mtr_symbol_table* table) {
    IMPLEMENT
}

static void walk_while(struct mtr_while* stmt, struct mtr_symbol_table* table) {
    IMPLEMENT
}

static void walk_block(struct mtr_block* stmt, struct mtr_symbol_table* table) {
    IMPLEMENT
}

static void walk_stmt(struct mtr_stmt* stmt, struct mtr_symbol_table* table) {
    switch (stmt->type)
    {
    case MTR_STMT_VAR_DECL: return walk_var((struct mtr_var_decl*) stmt, table);
    case MTR_STMT_FUNC:     return walk_fn((struct mtr_fn_decl*) stmt, table);
    case MTR_STMT_IF:       return walk_if((struct mtr_if*) stmt, table);
    case MTR_STMT_WHILE:    return walk_while((struct mtr_while*) stmt, table);
    case MTR_STMT_BLOCK:    return walk_block((struct mtr_block*) stmt, table);
    default:
        break;
    }
}

struct mtr_symbol_table mtr_load_symbols(struct mtr_ast ast) {
    struct mtr_symbol_table table = mtr_new_symbol_table();

    for (size_t i = 0; i < ast.size; ++i) {
        struct mtr_stmt* s = ast.statements + i;
        walk_stmt(s, &table);
    }

    return table;
}
