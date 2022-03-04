#ifndef NDEBUG

#ifndef MTR_DUMP_H
#define MTR_DUMP_H


#include "token.h"
#include "expr.h"
#include "symbol.h"
#include "stmt.h"

const char* mtr_token_type_to_str(enum mtr_token_type type);
const char* mtr_data_type_to_str(struct mtr_data_type type);

void mtr_dump_token(struct mtr_token token);
void mtr_dump_expr(struct mtr_expr* expr);
void mtr_dump_stmt(struct mtr_stmt* stmt);

#endif

#endif
