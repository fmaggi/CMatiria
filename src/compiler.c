#include "compiler.h"

struct mtr_compiler mtr_new_compiler_unit(const char* filepath) {
    struct mtr_file file = mtr_read_file(filepath);
    struct mtr_scanner scanner = {
        .source = file.bytes,
        .start = file.bytes,
        .current = file.bytes
    };

    struct mtr_compiler c = {
        .file = file,
        .scanner = scanner,
        .error_count = 0
    };

    return c;
}

bool mtr_compile(struct mtr_compiler* compiler) {
    return true;
}
