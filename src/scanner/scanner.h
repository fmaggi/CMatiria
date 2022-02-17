#ifndef _MTR_SCANNER_H
#define _MTR_SCANNER_H

#include "token.h"
#include "core/file.h"

struct mtr_scanner {
    const char* start;
    const char* current;
    u32 line;
};

struct mtr_scanner mtr_scanner_init(const char* source);

struct mtr_token_array mtr_scan(struct mtr_scanner* scanner);

struct mtr_token mtr_next_token(struct mtr_scanner* scanner);

#endif
