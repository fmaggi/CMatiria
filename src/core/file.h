#ifndef MTR_FILE_H
#define MTR_FILE_H

#include "types.h"

struct mtr_file {
    char* bytes;
    size_t size;
};

struct mtr_file mtr_read_file(const char* filepath);
void mtr_free_file(struct mtr_file file);

#endif
