#ifndef _MTR_ERROR_H
#define _MTR_ERROR_H

#include "compiler.h"

void mtr_scanner_error(struct mtr_scanner* scanner, const char* message, u32 index);

#endif
