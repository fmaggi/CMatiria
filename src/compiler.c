#include "compiler.h"

#include "package.h"
#include "scanner.h"
#include "parser.h"
#include "symbol.h"
#include "semantic_analysis.h"

#include "core/file.h"
#include "core/log.h"

#include <stdlib.h>
#include <string.h>

FILE* mtr_output_file = NULL;

bool mtr_compile(const char* source) {

    if (NULL == mtr_output_file) {
        mtr_output_file = stdout;
    }

    struct mtr_scanner scanner = mtr_scanner_init(source);
    struct mtr_parser parser = mtr_parser_init(scanner);

    struct mtr_ast ast = mtr_parse(&parser);

    if (parser.had_error)
        return false;

    struct mtr_package package = mtr_new_package(source, ast);
    bool all_ok = mtr_perform_semantic_analysis(&package);

    mtr_delete_symbol_table(&package.globals.symbols);
    mtr_delete_ast(&ast);
    return true;
}
