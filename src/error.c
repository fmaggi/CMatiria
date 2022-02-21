#include "error.h"

#include "core/log.h"
#include "core/types.h"

#include <string.h>

void mtr_report_error(struct mtr_token token, const char* message, const char* const source) {
    const char* c = source;
    const char* t = token.start;
    if (token.type == MTR_TOKEN_EOF) {
        --t;
        if (*t == '\n')
            --t;
    }

    // count lines
    u32 line = 1;
    while(c != t) {
        c++;
        if (*c == '\n')
            line++;
    }

    const char* line_start = t;
    while (*(line_start-1) != '\n' && line_start != source)
        --line_start;

    u32 column = t - line_start;

    // find the next new line so that it doesnt get printed
    const char* line_end = t;
    while (*line_end != '\n' && *line_end != '\0')
        ++line_end;

    u32 eol_index = line_end - line_start;

    MTR_LOG_ERROR("[%i:%i]: %s", line, column, message);
    MTR_LOG("\t%.*s", eol_index, line_start);

    char buf[256];
    memset(buf, ' ', 256);
    MTR_LOG(MTR_BOLD_DARK(MTR_GREEN) "\t%.*s%s" MTR_RESET, column, buf, "^---");
}
