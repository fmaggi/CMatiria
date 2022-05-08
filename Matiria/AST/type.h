#ifndef MTR_TYPE_H
#define MTR_TYPE_H

#include "scanner/token.h"
#include "core/types.h"

enum mtr_data_type {
    MTR_DATA_INVALID = 0,

    MTR_DATA_ANY,
    MTR_DATA_VOID,

    MTR_DATA_BOOL,
    MTR_DATA_INT,
    MTR_DATA_FLOAT,

    MTR_DATA_STRING,

    MTR_DATA_ARRAY,
    MTR_DATA_MAP,
    MTR_DATA_FN,
    MTR_DATA_USER,
    MTR_DATA_UNION,
    MTR_DATA_STRUCT,
};

typedef void mtr_object_type;

struct mtr_type {
    enum mtr_data_type type;
};

struct mtr_type mtr_get_data_type(struct mtr_token token);
bool mtr_is_compound_type(const struct mtr_type* type);

bool mtr_type_match(const struct mtr_type* lhs, const struct mtr_type* rhs);

struct mtr_type* mtr_get_underlying_type(const struct mtr_type* type);

struct mtr_array_type {
    struct mtr_type type;
    struct mtr_type* element;
};

struct mtr_map_type {
    struct mtr_type type;
    struct mtr_type* key;
    struct mtr_type* value;
};

struct mtr_function_type {
    struct mtr_type type;
    struct mtr_type* return_;
    struct mtr_type** argv;
    u8 argc;
};

struct mtr_user_type {
    struct mtr_type type;
    struct mtr_token name;
};

struct mtr_union_type {
    struct mtr_type type;
    struct mtr_user_type name;
    struct mtr_type** types;
    u16 argc;
};

struct mtr_struct_type {
    struct mtr_type type;
    struct mtr_user_type name;
    struct mtr_symbol** members;
    u16 argc;
};

#endif
