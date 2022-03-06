#ifndef MTR_SYMBOL_H
#define MTR_SYMBOL_H

#include "token.h"
#include "type.h"
#include "core/types.h"

struct mtr_type mtr_get_data_type(struct mtr_token token);

struct mtr_symbol {
    struct mtr_token token;
    struct mtr_type type;
    size_t index;
};

#endif
