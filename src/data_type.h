#ifndef _MTR_DATA_TYPE_H
#define _MTR_DATA_TYPE_H

enum mtr_data_type {
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

struct mtr_data {
    enum mtr_data_type type;
    const char* user_struct;
};

#endif
