#ifndef MTR_SYMBOL_H
#define MTR_SYMBOL_H

#include "type.h"
#include "scanner/token.h"
#include "core/types.h"

struct mtr_symbol {
    struct mtr_token token;
    struct mtr_type type;
    size_t index;
};

#endif
