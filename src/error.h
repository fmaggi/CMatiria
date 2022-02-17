#ifndef _MTR_ERROR_H
#define _MTR_ERROR_H

#include "scanner/token.h"

void mtr_report_error(struct mtr_token token, const char* message);

#endif
