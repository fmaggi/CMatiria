#ifndef MTR_SCANNER_H
#define MTR_SCANNER_H

#include "token.h"

struct mtr_scanner {
    const char* source;
    char* start;
    char* current;
};

struct mtr_token next_token(struct mtr_scanner* scanner);

#endif
