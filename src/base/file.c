#include "file.h"

#include <stdlib.h>

#include "log.h"

struct mtr_file mtr_read_file(const char* filepath) {

    struct mtr_file f = {
        .bytes = NULL,
        .size = 0
    };

    FILE* file = fopen(filepath, "r");
    if (!file)
    {
        MTR_LOG_ERROR("Unable to open file at %s", filepath);
        return f;
    }

    fseek(file, 0L, SEEK_END);
    size_t size = ftell(file);
    rewind(file);
    char* bytes = malloc(size * sizeof(char) + 1);
    size_t actuallyRead = fread(bytes, 1, size, file);
    if (actuallyRead != size)
    {
        MTR_LOG_ERROR("Invalid file read from %s", filepath);
        free(bytes);
        return f;
    }

    fclose(file);
    bytes[size] = 0;

    f.bytes = bytes;
    f.size = size+1;

    return f;
}
