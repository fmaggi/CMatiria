#ifndef _MTR_PARSER_H
#define _MTR_PARSER_H

#include "scanner/scanner.h"
#include "ast_nodes.h"

#include "core/types.h"

struct mtr_parser {
    struct mtr_scanner scanner;
    struct mtr_token token;
    struct mtr_expr_array array;
};

struct mtr_parser mtr_parser_init(struct mtr_scanner scanner);
void mtr_parser_shutdown(struct mtr_parser* parser);

struct mtr_expr* mtr_parse(struct mtr_parser* parser);

#endif
