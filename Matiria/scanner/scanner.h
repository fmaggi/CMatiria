#ifndef MTR_SCANNER_H
#define MTR_SCANNER_H

#include "token.h"
#include "core/file.h"

struct mtr_scanner {
    const char* source;
    const char* start;
    const char* current;
};

void mtr_scanner_init(struct mtr_scanner* scanner, const char* source);

struct mtr_token mtr_next_token(struct mtr_scanner* scanner);

#endif
