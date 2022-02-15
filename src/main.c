#include <stdio.h>

#include "base/file.h"
#include "base/log.h"

#include "scanner.h"
#include "compiler.h"
#include "string.h"

int main(int argc, char* argv[])
{
    struct mtr_compiler compiler = mtr_new_compiler_unit(argv[1]);

    struct mtr_token_array array = mtr_scan(&(compiler.scanner));

    for (size_t i = 0; i < array.size; ++i) {
        mtr_print_token(array.tokens[i]);
    }

    mtr_free_file(compiler.file);
    mtr_delete_array(&array);
}
