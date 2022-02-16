#ifndef _MTR_PARSER_H
#define _MTR_PARSER_H

#include "scanner/scanner.h"
#include "ast_nodes.h"

struct mtr_parser {
    struct mtr_scanner scanner;
    struct mtr_token token;
};

struct mtr_expr* mtr_parse(struct mtr_parser* parser);

void mtr_print_expr(struct mtr_expr* node);

#endif
