#ifndef MTR_SCANNER_H
#define MTR_SCANNER_H

#include "token.h"
#include "base/file.h"

struct mtr_scanner {
    const char* source;
    char* start;
    char* current;
};

struct mtr_token_array mtr_scan(struct mtr_scanner* scanner);

struct mtr_token mtr_next_token(struct mtr_scanner* scanner);

#endif
