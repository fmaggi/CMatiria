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

    advance(&parser);

    while (parser.token.type != MTR_TOKEN_EOF) {
        struct mtr_stmt* stmt = mtr_parse(&parser);

        if (!parser.had_error)
            mtr_print_expr(stmt->expression);


        mtr_free_expr(stmt->expression);
        free(stmt);
    }

    // mtr_print_token(parser.token);

    free(source);
    return true;
}
