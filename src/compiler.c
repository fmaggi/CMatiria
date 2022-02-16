#include "compiler.h"

#include "scanner.h"

#include "core/file.h"

bool mtr_compile(const char* filepath) {

    struct mtr_file file = mtr_read_file(filepath);

    struct mtr_scanner scanner = {
        .start = file.bytes,
        .current = file.bytes,
        .source = file.bytes
    };

    struct mtr_token_array array = mtr_scan(&scanner);

    for (size_t i = 0; i < array.size; ++i) {
        mtr_print_token(array.tokens[i]);
    }

    mtr_delete_array(&array);

    mtr_free_file(file);
    return true;
}
