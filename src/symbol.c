#include "symbol.h"

#include "core/log.h"

#include <stdlib.h>
#include <string.h>

#define LOAD_FACTOR 0.75

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
    for (size_t i = 0; i < table->capacity; ++i) {
        struct mtr_entry* old = table->entries + i;
        if (old->key == NULL)
            continue;
        free(old->key);
    }

    table->entries = 0;
    table->size = 0;
    free(table->entries);
    table->entries = NULL;
}

static u32 hash(const char* key, size_t length) {
  u32 hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (u8)key[i];
    hash *= 16777619;
  }
  return hash;
}

static struct mtr_entry* find_entry(struct mtr_entry* entries, const char* key, size_t length, size_t cap) {
    u32 hash_ = hash(key, length);
    u32 index = hash_ % cap;

    struct mtr_entry* entry = entries + index;

    while (entry->key != NULL) {
        if (hash(entry->key, strlen(entry->key)) == hash_)
            break;
        index = (index + 1) % cap;
        entry = entries + index;
    }

    return entry;
}

void mtr_insert_symbol(struct mtr_symbol_table* table, const char* key, size_t length, struct mtr_symbol symbol) {
    struct mtr_entry* entry = find_entry(table->entries, key, length, table->capacity);
    entry->symbol = symbol;
    if (entry->key != NULL) {
        return;
    }
    entry->key = malloc(length+1);
    memcpy(entry->key, key, length);
    entry->key[length] = 0;

    table->size += 1;
    if (table->size >= table->capacity * LOAD_FACTOR) {
        struct mtr_entry* old_entries = table->entries;
        size_t new_cap = table->capacity * 2;

        void* temp = calloc(new_cap, sizeof(struct mtr_entry));
        if (NULL == temp) {
            MTR_LOG_ERROR("Bad allocation.");
            free(old_entries);
            return;
        }

        for (size_t i = 0; i < table->capacity; ++i) {
            struct mtr_entry* old = table->entries + i;
            if (old->key == NULL)
                continue;
            struct mtr_entry* entry = find_entry(temp, old->key, strlen(old->key), new_cap);
            entry->key = old->key;
            entry->symbol = old->symbol;
        }

        free(old_entries);
        table->capacity = new_cap;
        table->entries = temp;
    }
}

struct mtr_symbol* mtr_get_symbol(const struct mtr_symbol_table* table, const char* key, size_t length) {
    struct mtr_entry* entry = find_entry(table->entries, key, length, table->capacity);
    if (entry->key == NULL) {
        return NULL;
    }
    return &entry->symbol;
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
