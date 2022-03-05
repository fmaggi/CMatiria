#ifndef MTR_SYMBOL_H
#define MTR_SYMBOL_H

#include "token.h"
#include "core/types.h"

#define BIT(x) 1 << x

enum mtr_data_type_e {
    MTR_DATA_INVALID = 0,

    MTR_DATA_USER_DEFINED = BIT(0),

    MTR_DATA_BOOL = BIT(1),
    MTR_DATA_INT = BIT(2),
    MTR_DATA_FLOAT = BIT(3),

    MTR_DATA_NUM = MTR_DATA_INT | MTR_DATA_FLOAT
};

#undef BIT


struct mtr_data_type {
    const char* user_struct;
    size_t length;
    enum mtr_data_type_e type;
};

struct mtr_data_type mtr_get_data_type(struct mtr_token token);

struct mtr_symbol {
    struct mtr_token token;
    struct mtr_data_type type;
    size_t index;
};

#endif
