#ifndef _MTR_SYMBOL_H
#define _MTR_SYMBOL_H

#include "stmt.h"

#include "core/types.h"

enum mtr_data_type_e {
    MTR_DATA_INVALID,
    MTR_DATA_U8,
    MTR_DATA_U16,
    MTR_DATA_U32,
    MTR_DATA_U64,
    MTR_DATA_I8,
    MTR_DATA_I16,
    MTR_DATA_I32,
    MTR_DATA_I64,
    MTR_DATA_F32,
    MTR_DATA_F64,
    MTR_DATA_BOOL,
    MTR_DATA_USER_DEFINED
};

enum mtr_data_type_e mtr_get_data_type(enum mtr_token_type type);

struct mtr_data_type {
    enum mtr_data_type_e type;
    const char* user_struct;
};

struct mtr_symbol {
    struct mtr_data_type type;
    union {
        u64 integer;
        f64 floating;
        bool boolean;
        char* user_defined;
    } as;
    bool is_callable;
};

struct mtr_entry {
    struct mtr_symbol symbol;
    const char* key;
    size_t length;
};

struct mtr_symbol_table {
    struct mtr_entry* entries;
    size_t size;
    size_t capacity;
};

struct mtr_symbol_table mtr_new_symbol_table();
void mtr_delete_symbol_table(struct mtr_symbol_table* table);

void mtr_insert_symbol(struct mtr_symbol_table* table, const char* key, size_t length, struct mtr_symbol symbol);
struct mtr_symbol* mtr_get_symbol(const struct mtr_symbol_table* table, const char* key, size_t length);
void mtr_delete_symbol(const struct mtr_symbol_table* table, const char* key, size_t length);

struct mtr_symbol_table mtr_load_symbols(struct mtr_ast ast);

#endif
