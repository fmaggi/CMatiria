#ifndef _MTR_COMPILER_H
#define _MTR_COMPILER_H

#include "scanner.h"
#include "base/file.h"

#include "base/types.h"

struct mtr_compiler {
    struct mtr_scanner scanner;
    struct mtr_file file;

    u32 error_count;
};

struct mtr_compiler mtr_new_compiler_unit(const char* filepath);

bool mtr_compile(struct mtr_compiler* compiler);

#endif
