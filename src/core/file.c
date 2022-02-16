#include "file.h"

#include "log.h"

#include <stdio.h>
#include <stdlib.h>

char* mtr_read_file(const char* filepath) {

    FILE* file = fopen(filepath, "r");
    if (NULL == file) {
        MTR_LOG_ERROR("Unable to open file at %s", filepath);
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    char* bytes = malloc(size * sizeof(char) + 1);
    if (NULL == bytes) {
        MTR_LOG_ERROR("Bad allocation! (%lu bytes for file %s)", size * sizeof(char) + 1, filepath);
        fclose(file);
        return NULL;
    }

    size_t actuallyRead = fread(bytes, 1, size, file);
    if (actuallyRead != size) {
        MTR_LOG_ERROR("Invalid file read from %s", filepath);
        free(bytes);
        fclose(file);
        return NULL;
    }

    fclose(file);
    bytes[size] = 0;

    return bytes;

}
