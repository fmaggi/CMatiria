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

    struct mtr_program program = mtr_parse(&parser);

    mtr_print_expr(program.declarations[0].statement.expression);

    free(program.declarations);
    free(source);
    return true;
}
