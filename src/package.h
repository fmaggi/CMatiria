#ifndef _MTR_PACKAGE_H
#define _MTR_PACKAGE_H

#include "stmt.h"
#include "bytecode.h"

struct mtr_globals {
    struct gloabl_entry* entries;
    size_t size;
    size_t capacity;
};

struct mtr_package {
    struct mtr_globals indices;
    struct mtr_chunk* functions;

    const char* source;
};

struct mtr_package mtr_new_package(const char* const source, struct mtr_ast ast);

#endif
