#ifndef _MTR_SYMBOL_H
#define _MTR_SYMBOL_H

#include "token.h"
#include "core/types.h"

enum mtr_data_type_e {
    MTR_DATA_INVALID,
    MTR_DATA_INT,
    MTR_DATA_FLOAT,
    MTR_DATA_BOOL,
    MTR_DATA_USER_DEFINED
};

enum mtr_data_type_e mtr_get_data_type(enum mtr_token_type type);


struct mtr_data_type {
    const char* user_struct;
    size_t length;
    enum mtr_data_type_e type;
};

bool mtr_data_type_match(struct mtr_data_type lhs, struct mtr_data_type rhs);

struct mtr_symbol {
    struct mtr_token token;
    struct mtr_data_type type;
};

#endif
