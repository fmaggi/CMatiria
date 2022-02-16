#ifndef _MTR_ERROR_H
#define _MTR_ERROR_H

#include "scanner.h"

#include "core/types.h"

void mtr_scanner_error(struct mtr_scanner* scanner, const char* message, u32 index);

#endif
