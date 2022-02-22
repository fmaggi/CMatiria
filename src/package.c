#include "package.h"

struct mtr_package mtr_new_package(const char* const source, struct mtr_ast ast) {
    struct mtr_package package = {
        .source = source,
        .ast = ast,
        .global_scope = mtr_new_scope(NULL)
    };
    return package;
}
