#include "error.h"

#include "core/log.h"
#include "core/types.h"

#include <string.h>

void mtr_report_error(struct mtr_token token, const char* message) {

    const char* source_start = token.start - token.char_idx;
    const char* c = source_start;

    // count lines
    u32 line = 1;
    while(c != token.start) {
        c++;
        if (*c == '\n')
            line++;
    }

    const char* line_start = token.start-1;
    while (*line_start != '\n' && line_start != source_start)
        --line_start;
    ++line_start;

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
    memset(marker_buf + marker_offset + 1, '-', 4);

    MTR_LOG_ERROR("[%i:%i]: %s", line, column, message);
    MTR_LOG("\t%s", line_buf);
    MTR_LOG(MTR_BOLD_DARK(MTR_GREEN) "\t%s" MTR_RESET, marker_buf);
}
