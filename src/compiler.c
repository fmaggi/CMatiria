#include "compiler.h"

#include "scanner.h"
#include "parser.h"
#include "symbol.h"
#include "sema/type_analysis.h"

#include "core/file.h"
#include "core/log.h"

#include <stdlib.h>
#include <string.h>

bool mtr_compile(const char* source) {
    struct mtr_scanner scanner = mtr_scanner_init(source);
    struct mtr_parser parser = mtr_parser_init(scanner);

    struct mtr_ast ast = mtr_parse(&parser);

    struct mtr_symbol_table table = mtr_load_symbols(ast);

    mtr_delete_symbol_table(&table);

    mtr_delete_ast(&ast);
    return true;
}
