#ifndef _MTR_ERROR_H
#define _MTR_ERROR_H

#include "token.h"

void mtr_report_error(struct mtr_token token, const char* message, const char* const source);
void mtr_report_warning(struct mtr_token token, const char* message, const char* const source);
void mtr_report_message(struct mtr_token token, const char* message, const char* const source);

#endif
