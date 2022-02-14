#include "file.h"

#include "stdlib.h"

#include "log.h"

char* read_file(const char* filepath) {
    FILE* file = fopen(filepath, "r");
    if (!file)
    {
        LOG_ERROR("Unable to open file at %s", filepath);
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    size_t size = ftell(file);
    rewind(file);
    char* bytes = malloc(size * sizeof(char) + 1);
    size_t actuallyRead = fread(bytes, 1, size, file);
    if (actuallyRead != size)
    {
        LOG_ERROR("Invalid file read from %s", filepath);
        return NULL;
    }

    fclose(file);
    bytes[size] = 0;

    return bytes;
}
