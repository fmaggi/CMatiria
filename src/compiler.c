#include "compiler.h"

#include "scanner/scanner.h"
#include "parser/parser.h"

#include "core/file.h"

#include <stdlib.h>

bool mtr_compile(const char* filepath) {

    char* source = mtr_read_file(filepath);

    struct mtr_scanner scanner = mtr_scanner_init(source);
    struct mtr_parser parser = mtr_parser_init(scanner);

    struct mtr_expr* expr = mtr_parse(&parser);
    mtr_print_expr(expr);

    mtr_parser_shutdown(&parser);

    free(source);
    return true;
}
