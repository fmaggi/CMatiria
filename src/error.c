#include "error.h"

#include "core/log.h"
#include "core/types.h"

#include <string.h>

void mtr_report_error(struct mtr_token token, const char* message) {

    const char* line_start = token.start;
    while (*(line_start-1) != '\n')
        --line_start;

    u32 column = token.start - line_start;
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

    MTR_LOG_ERROR("[%i:%i]: %s", token.line, column, message);
    MTR_LOG("\t%s\n\t%s", line_buf, marker_buf);
}
