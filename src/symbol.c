#include "symbol.h"

#include "core/log.h"

#include <string.h>

enum mtr_data_type_e mtr_get_data_type(enum mtr_token_type type) {
    switch (type)
    {
    case MTR_TOKEN_INT_LITERAL:
    case MTR_TOKEN_INT:
        return MTR_DATA_INT;

    case MTR_TOKEN_FLOAT_LITERAL:
    case MTR_TOKEN_FLOAT:
        return MTR_DATA_FLOAT;

    case MTR_TOKEN_BOOL:
    case MTR_TOKEN_TRUE:
    case MTR_TOKEN_FALSE:

    case MTR_TOKEN_LESS:
    case MTR_TOKEN_LESS_EQUAL:
    case MTR_TOKEN_GREATER:
    case MTR_TOKEN_GREATER_EQUAL:
    case MTR_TOKEN_EQUAL_EQUAL:
    case MTR_TOKEN_BANG_EQUAL:
        return MTR_DATA_BOOL;

    case MTR_TOKEN_IDENTIFIER:
        return MTR_DATA_USER_DEFINED;
    default:
        break;
    }
    // MTR_LOG_WARN("Invalid data type");
    return MTR_DATA_INVALID;
}

bool mtr_data_type_match(struct mtr_data_type lhs, struct mtr_data_type rhs) {
    if (lhs.type != rhs.type) {
        return false;
    }

    if (lhs.type != MTR_DATA_USER_DEFINED) {
        return true;
    }

    return (lhs.length == rhs.length) && (memcmp(lhs.user_struct, rhs.user_struct, lhs.length) == 0);
}
