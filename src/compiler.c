#include "compiler.h"

#include "scanner.h"
#include "parser.h"
#include "interpret.h"
#include "sema/type_analysis.h"

#include "core/file.h"
#include "core/log.h"

#include <stdlib.h>

bool mtr_compile(const char* source) {
    struct mtr_scanner scanner = mtr_scanner_init(source);
    struct mtr_parser parser = mtr_parser_init(scanner);

    struct mtr_ast ast = mtr_parse(&parser);

    if (!parser.had_error) {
        bool x = mtr_type_check(ast);
        MTR_LOG_DEBUG("%u", x);
    }

    mtr_interpret(ast);

    mtr_delete_ast(&ast);
    return true;
}
