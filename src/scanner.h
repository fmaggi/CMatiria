#ifndef MTR_SCANNER_H
#define MTR_SCANNER_H

#include "token.h"
#include "core/file.h"

struct mtr_scanner {
    const char* const source;
    const char* start;
    const char* current;
};

struct mtr_scanner mtr_scanner_init(const char* source);

struct mtr_token mtr_next_token(struct mtr_scanner* scanner);

#endif
