#ifndef _MTR_ERROR_H
#define _MTR_ERROR_H

#include "token.h"

void mtr_report_error(const char* token, const char* message, const char* const source);

#endif
