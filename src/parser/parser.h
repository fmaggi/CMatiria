#ifndef MTR_PARSER_H
#define MTR_PARSER_H

#include "scanner/scanner.h"
#include "AST/stmt.h"

#include "core/types.h"

struct mtr_parser {
    struct mtr_scanner scanner;
    struct mtr_token token;
    struct mtr_function_decl* current_function;
    bool had_error;
    bool panic;
};

struct mtr_parser mtr_parser_init(struct mtr_scanner scanner);

struct mtr_ast mtr_parse(struct mtr_parser* parser);

void mtr_delete_ast(struct mtr_ast* ast);

#endif
