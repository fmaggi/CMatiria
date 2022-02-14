#ifndef MTR_FILE_H
#define MTR_FILE_H

#include <stdio.h>
#include "types.h"

struct mtr_file {
    char* bytes;
    size_t size;
};

struct mtr_file mtr_read_file(const char* filepath);

#endif
