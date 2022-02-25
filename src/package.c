#include "package.h"

struct mtr_package mtr_new_package(const char* const source, struct mtr_ast ast) {
    struct mtr_package package = {
        .source = source,
    };
    return package;
}
