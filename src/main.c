#include <stdio.h>

#include "base/file.h"
#include "base/log.h"

#include "scanner.h"
#include "string.h"

int main(int argc, char* argv[])
{
    struct mtr_file file = mtr_read_file(argv[1]);

    struct mtr_scanner scanner = {
        .current = file.bytes,
        .source = file.bytes,
        .start = file.bytes
    };

    struct mtr_token_array array = mtr_scan(&scanner);

    for (size_t i = 0; i < array.size; ++i) {
        mtr_print_token(array.tokens[i]);
    }

    mtr_free_file(file);
    mtr_delete_array(&array);
}
