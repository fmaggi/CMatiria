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

    // find the next new line so that it doesnt get printed
    const char* line_end = line_start;
    while (*line_end != '\n')
        ++line_end;

    u32 eol_index = line_end - line_start;

    MTR_LOG_ERROR("[%i:%i]: %s", line, column, message);
    MTR_LOG("\t%.*s", eol_index, line_start);

    char buf[256];
    memset(buf, ' ', 256);
    MTR_LOG(MTR_BOLD_DARK(MTR_GREEN) "\t%.*s%s" MTR_RESET, column, buf, "^---");

}
