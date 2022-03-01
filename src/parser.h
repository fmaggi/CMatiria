#ifndef _MTR_PARSER_H
#define _MTR_PARSER_H

#include "scanner.h"
#include "expr.h"
#include "stmt.h"

#include "core/types.h"

struct mtr_parser {
    struct mtr_scanner scanner;
    struct mtr_token token;
    bool had_error;
    bool panic;
};

struct mtr_parser mtr_parser_init(struct mtr_scanner scanner);

struct mtr_ast mtr_parse(struct mtr_parser* parser);

void mtr_delete_ast(struct mtr_ast* ast);

#endif
