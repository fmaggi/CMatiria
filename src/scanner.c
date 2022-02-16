#include "scanner.h"

#include "core/log.h"
#include "error.h"

#include <string.h>

struct keyword_entry {
    const enum mtr_token_type type;
    const char* str;
    const size_t str_len;
};

#define FIRST_KEYWORD MTR_TOKEN_STRUCT
#define LAST_KEYWORD  MTR_TOKEN_BOOL
#define KEYWORD_COUNT LAST_KEYWORD - FIRST_KEYWORD + 1

static const struct keyword_entry keywords[KEYWORD_COUNT] = {
    { .type = MTR_TOKEN_STRUCT, .str = "struct", .str_len = strlen("struct") },
    { .type = MTR_TOKEN_IF,     .str = "if",     .str_len = strlen("if")     },
    { .type = MTR_TOKEN_ELSE,   .str = "else",   .str_len = strlen("else")   },
    { .type = MTR_TOKEN_TRUE,   .str = "true",   .str_len = strlen("true")   },
    { .type = MTR_TOKEN_FALSE,  .str = "false",  .str_len = strlen("false")  },
    { .type = MTR_TOKEN_FN,     .str = "fn",     .str_len = strlen("fn")     },
    { .type = MTR_TOKEN_RETURN, .str = "return", .str_len = strlen("return") },
    { .type = MTR_TOKEN_WHILE,  .str = "while",  .str_len = strlen("while")  },
    { .type = MTR_TOKEN_FOR,    .str = "for",    .str_len = strlen("for")    },
    { .type = MTR_TOKEN_U8,     .str = "u8",     .str_len = strlen("u8")     },
    { .type = MTR_TOKEN_U16,    .str = "u16",    .str_len = strlen("u16")    },
    { .type = MTR_TOKEN_U32,    .str = "u32",    .str_len = strlen("u32")    },
    { .type = MTR_TOKEN_U64,    .str = "u64",    .str_len = strlen("u64")    },
    { .type = MTR_TOKEN_I8,     .str = "i8" ,    .str_len = strlen("i8")     },
    { .type = MTR_TOKEN_I16,    .str = "i16",    .str_len = strlen("i16")    },
    { .type = MTR_TOKEN_I32,    .str = "i32",    .str_len = strlen("i32")    },
    { .type = MTR_TOKEN_I64,    .str = "i64",    .str_len = strlen("i64")    },
    { .type = MTR_TOKEN_F32,    .str = "f32",    .str_len = strlen("f32")    },
    { .type = MTR_TOKEN_F64,    .str = "f64",    .str_len = strlen("f64")    },
    { .type = MTR_TOKEN_BOOL,   .str = "bool",   .str_len = strlen("bool")   },
};

static const struct mtr_token invalid_token = {
    .type = MTR_TOKEN_INVALID,
    .start = NULL,
    .char_index = 0,
    .length = 0
};

static bool is_numeric(char c);
static bool is_alpha(char c);
static bool is_alphanumeric(char c);

static char advance(struct mtr_scanner* scanner);
static char peek(char* c);
static char* skip_whitespace(char* string);

static struct mtr_token make_token(const struct mtr_scanner* scanner, enum mtr_token_type type);

static struct mtr_token scan_string(struct mtr_scanner* scanner);
static struct mtr_token scan_number(struct mtr_scanner* scanner);
static struct mtr_token scan_identifier(struct mtr_scanner* scanner);
static struct mtr_token scan_comment(struct mtr_scanner* scanner);

struct mtr_token_array mtr_scan(struct mtr_scanner* scanner) {
    struct mtr_token_array tokens = mtr_new_array(10);
    while (*scanner->current != '\0') {
        struct mtr_token t = mtr_next_token(scanner);
        if (t.type == MTR_TOKEN_INVALID)
            continue;
        mtr_insert_token(&tokens, t);
    }

    struct mtr_token e = {
        .type = MTR_TOKEN_EOF,
        .char_index = 0,
        .start = NULL,
        .length = 0
    };

    mtr_insert_token(&tokens, e);

    return tokens;
}

struct mtr_token mtr_next_token(struct mtr_scanner* scanner) {
    scanner->current = skip_whitespace(scanner->current);
    scanner->start = scanner->current;

    char c = advance(scanner);

    switch (c)
    {
    case '+': return make_token(scanner, MTR_TOKEN_PLUS);

    case '-':
        if (peek(scanner->current) == '>') {
            advance(scanner);
            return make_token(scanner, MTR_TOKEN_ARROW);
        }
        return make_token(scanner, MTR_TOKEN_MINUS);

    case '*': return make_token(scanner, MTR_TOKEN_STAR);

    case '/':
        if (peek(scanner->current) == '/') {
            advance(scanner);
            return scan_comment(scanner);
        }
        return make_token(scanner, MTR_TOKEN_SLASH);
    case '%': return make_token(scanner, MTR_TOKEN_PERCENT);
    case ',': return make_token(scanner, MTR_TOKEN_COMMA);
    case ':': return make_token(scanner, MTR_TOKEN_COLON);
    case ';': return make_token(scanner, MTR_TOKEN_SEMICOLON);
    case '.': return make_token(scanner, MTR_TOKEN_DOT);
    case '(': return make_token(scanner, MTR_TOKEN_PAREN_L);
    case ')': return make_token(scanner, MTR_TOKEN_PAREN_R);
    case '[': return make_token(scanner, MTR_TOKEN_SQR_L);
    case ']': return make_token(scanner, MTR_TOKEN_SQR_R);
    case '{': return make_token(scanner, MTR_TOKEN_CURLY_L);
    case '}': return make_token(scanner, MTR_TOKEN_CURLY_R);

    case '!':
        if (peek(scanner->current) == '=') {
            advance(scanner);
            return make_token(scanner, MTR_TOKEN_BANG_EQUAL);
        }
        return make_token(scanner, MTR_TOKEN_BANG);

    case '=':
        if (peek(scanner->current) == '=') {
            advance(scanner);
            return make_token(scanner, MTR_TOKEN_EQUAL_EQUAL);
        }
        return make_token(scanner, MTR_TOKEN_EQUAL);

    case '>':
        if (peek(scanner->current) == '=') {
            advance(scanner);
            return make_token(scanner, MTR_TOKEN_GREATER_EQUAL);
        }
        return make_token(scanner, MTR_TOKEN_GREATER);

    case '<':
        if (peek(scanner->current) == '=') {
            advance(scanner);
            return make_token(scanner, MTR_TOKEN_LESS_EQUAL);
        }
        return make_token(scanner, MTR_TOKEN_LESS);

    case '&':
        if (peek(scanner->current) == '&') {
            advance(scanner);
            return make_token(scanner, MTR_TOKEN_AND);
        }
        return invalid_token;

    case '|':
        if (peek(scanner->current) == '|') {
            advance(scanner);
            return make_token(scanner, MTR_TOKEN_OR);
        }
        return invalid_token;

    case '"': return scan_string(scanner);

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        return scan_number(scanner);

    case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o':
    case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z':

    case 'A': case 'B': case 'C': case 'D': case 'E':
    case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z':

    case '_':
        return scan_identifier(scanner);

    case '\n':
        return invalid_token;
    }
    mtr_scanner_error(scanner, "Invalid token", scanner->start - scanner->source);

    return invalid_token;
}

static bool is_numeric(char c) {
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_alphanumeric(char c) {
    return is_numeric(c) || is_alpha(c);
}

static char advance(struct mtr_scanner* scanner) {
    return *(scanner->current++);
}

static char peek(char* c) {
    return *c;
}

static char* skip_whitespace(char* string) {
    while(*string == ' ')
        string++;
    return string;
}

static struct mtr_token make_token(const struct mtr_scanner* scanner, enum mtr_token_type type) {
    struct mtr_token t = {
        .start = scanner->start,
        .char_index = scanner->start - scanner->source,
        .length = scanner->current - scanner->start,
        .type = type
    };
    return t;
}

static struct mtr_token scan_string(struct mtr_scanner* scanner) {
    while (peek(scanner->current) != '"')
        advance(scanner);
    advance(scanner); // closing "
    return make_token(scanner, MTR_TOKEN_STRING);
}

static struct mtr_token scan_number(struct mtr_scanner* scanner) {
    while (is_numeric(peek(scanner->current)))
        advance(scanner);

    if (peek(scanner->current) == '.') {
        advance(scanner);
        while (is_numeric(peek(scanner->current)))
            advance(scanner);
        return make_token(scanner, MTR_TOKEN_FLOAT);
    }

    return make_token(scanner, MTR_TOKEN_INT);
}

static bool comp_identifier(const char* k, char* s) {
    size_t max = strlen(k);
    for (size_t i = 0; i < max; ++i) {
        if (k[i] != *s)
            return false;
        ++s;
    }
    return true;
}

static struct mtr_token scan_identifier(struct mtr_scanner* scanner) {
    while (is_alphanumeric(peek(scanner->current)))
        advance(scanner);

    for (int i = 0; i < KEYWORD_COUNT; ++i) {
        const struct keyword_entry k = keywords[i];
        if ((scanner->current - scanner->start) == k.str_len && comp_identifier(k.str, scanner->start))
            return make_token((scanner), k.type);
    }

    return make_token(scanner, MTR_TOKEN_IDENTIFIER);
}

static struct mtr_token scan_comment(struct mtr_scanner* scanner) {
    while (peek(scanner->current) != '\n')
        advance(scanner);
    return invalid_token;
}

