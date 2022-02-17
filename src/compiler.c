#include "compiler.h"

#include "scanner/scanner.h"
#include "parser/parser.h"

#include "core/file.h"
#include "core/log.h"

#include <stdlib.h>

bool mtr_compile(const char* filepath) {
    char* source = mtr_read_file(filepath);

    struct mtr_scanner scanner = mtr_scanner_init(source);
    struct mtr_parser parser = mtr_parser_init(scanner);

    // MTR_LOG_DEBUG("%p %zu", parser.array.expressions, parser.array.capacity);

    struct mtr_expr* expr = mtr_parse(&parser);

    if (!parser.had_error)
        mtr_print_expr(expr);

    free(source);
    return true;
}
