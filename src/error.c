#include "error.h"

#include "compiler.h"

#include "core/macros.h"
#include "core/log.h"

#include <string.h>

void mtr_scanner_error(struct mtr_scanner* scanner, const char* message, u32 index) {
    struct mtr_compiler* compiler = mtr_container_of(scanner, struct mtr_compiler, scanner);

    compiler->error_count++;

    const char* c = scanner->source;
    const char* at = c + index;
    u32 line = 1;
    const char* line_start = c;
    // count number of lines
    while (c != at) {
        if (*c == '\n') {
            line++;
            line_start = c + 1;
        }
        c++;
    }

    u32 column = at - line_start;
    u32 marker_offset = column;

    // make sure it fits in the buffer
    while (marker_offset > 32) {
        marker_offset -= 5;
        line_start += 5;
    }

    // find the next new line so that it doesnt get printed
    const char* line_end = line_start;
    while (*line_end != '\n')
        ++line_end;

    u32 eol_index = line_end - line_start;

    char line_buf[32] = {0};
    memcpy(line_buf, line_start, eol_index);
    line_buf[eol_index] = '\0';

    char marker_buf[32] = {0};
    memset(marker_buf, ' ', marker_offset);
    marker_buf[marker_offset] = '^';

    MTR_LOG_ERROR("[%i:%i]: %s", line, column, message);
    MTR_LOG("\t%s\n\t%s", line_buf, marker_buf);
}
