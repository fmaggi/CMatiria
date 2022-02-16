#include "compiler.h"

#include "scanner/scanner.h"

#include "core/file.h"

#include <stdlib.h>

bool mtr_compile(const char* filepath) {

    char* source = mtr_read_file(filepath);

    struct mtr_scanner scanner = {
        .start = source,
        .current = source,
        .source = source
    };

    struct mtr_token_array array = mtr_scan(&scanner);

    for (size_t i = 0; i < array.size; ++i) {
        mtr_print_token(array.tokens[i]);
    }

    mtr_delete_array(&array);

    free(source);
    return true;
}
