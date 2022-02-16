#include "compiler.h"

#include "scanner/scanner.h"
#include "parser/parser.h"

#include "core/file.h"

#include <stdlib.h>

bool mtr_compile(const char* filepath) {

    char* source = mtr_read_file(filepath);

    struct mtr_scanner scanner = {
        .start = source,
        .current = source,
        .source = source
    };

    struct mtr_parser parser = {
        .scanner = scanner
    };

    struct mtr_expr* expr = mtr_parse(&parser);
    mtr_print_expr(expr);

    free(source);
    return true;
}
