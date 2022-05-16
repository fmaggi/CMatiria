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

#endif
