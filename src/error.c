#include "error.h"

#include "core/log.h"
#include "core/types.h"

#include <string.h>

struct report {
    u32 line;
    u32 column;
    u32 eol_index;
    const char* line_start;
};

static struct report locate(struct mtr_token token, const char* const source) {
    const char* c = source;
    const char* t = token.start;
    if (*token.start == '\0') {
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

    struct report r;
    r.line = line;
    r.column = column;
    r.eol_index = eol_index;
    r.line_start = line_start;
    return r;
}

void mtr_report_error(struct mtr_token token, const char* message, const char* const source) {
    struct report r = locate(token, source);

    MTR_LOG_ERROR("[%i:%i]: %s", r.line, r.column, message);
    MTR_LOG("\t%.*s", r.eol_index, r.line_start);

    char buf[256];
    memset(buf, ' ', 256);
    MTR_LOG(MTR_BOLD_DARK(MTR_GREEN) "\t%.*s%s" MTR_RESET, r.column, buf, "^---");
}

void mtr_report_warning(struct mtr_token token, const char* message, const char* const source) {
    struct report r = locate(token, source);

    MTR_LOG_WARN("[%i:%i]: %s", r.line, r.column, message);
    MTR_LOG("\t%.*s", r.eol_index, r.line_start);

    char buf[256];
    memset(buf, ' ', 256);
    MTR_LOG(MTR_BOLD_DARK(MTR_GREEN) "\t%.*s%s" MTR_RESET, r.column, buf, "^---");
}

void mtr_report_message(struct mtr_token token, const char* message, const char* const source) {
    struct report r = locate(token, source);

    MTR_LOG_INFO("[%i:%i]: %s", r.line, r.column, message);
    MTR_LOG("\t%.*s", r.eol_index, r.line_start);

    char buf[256];
    memset(buf, ' ', 256);
    MTR_LOG(MTR_BOLD_DARK(MTR_GREEN) "\t%.*s%s" MTR_RESET, r.column, buf, "^---");
}
