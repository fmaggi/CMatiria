#ifndef MTR_SCOPE_H
#define MTR_SCOPE_H

#include "AST/symbol.h"

struct mtr_symbol_table {
    struct symbol_entry* entries;
    size_t size;
    size_t capacity;
};

void mtr_init_symbol_table(struct mtr_symbol_table* table);
void mtr_delete_symbol_table(struct mtr_symbol_table* table);

void mtr_symbol_table_insert(struct mtr_symbol_table* table, const char* key, size_t length, struct mtr_symbol symbol);
struct mtr_symbol* mtr_symbol_table_get(const struct mtr_symbol_table* table, const char* key, size_t length);
void mtr_symbol_table_remove(const struct mtr_symbol_table* table, const char* key, size_t length);

struct mtr_scope {
    struct mtr_symbol_table symbols;
    struct mtr_scope* parent;
    size_t current;
};

void mtr_init_scope(struct mtr_scope* scope, struct mtr_scope* parent);
void mtr_delete_scope(struct mtr_scope* scope);

struct mtr_symbol* mtr_scope_find(const struct mtr_scope* scope, struct mtr_token token);
struct mtr_symbol* mtr_scope_add(struct mtr_scope* scope, struct mtr_symbol symbol);

#endif
