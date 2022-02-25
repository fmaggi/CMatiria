#include "scanner.h"

#include <string.h>

struct mtr_scanner mtr_scanner_init(const char* source) {
    struct mtr_scanner scanner = {
        .source  = source,
        .start   = source,
        .current = source
    };
    return scanner;
}

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
    { .type = MTR_TOKEN_INT,    .str = "Int",    .str_len = strlen("Int")    },
    { .type = MTR_TOKEN_FLOAT,  .str = "Float",  .str_len = strlen("Float")  },
    { .type = MTR_TOKEN_BOOL,   .str = "Bool",   .str_len = strlen("Bool")   },
};

const struct mtr_token invalid_token = {
    .type = MTR_TOKEN_INVALID,
    .start = NULL,
    .length = 0
};

static bool is_numeric(char c);
static bool is_alpha(char c);
static bool is_alphanumeric(char c);

static char advance(struct mtr_scanner* scanner);
static void skip_whitespace(struct mtr_scanner* scanner);

static struct mtr_token make_token(const struct mtr_scanner* scanner, enum mtr_token_type type);

static struct mtr_token scan_string(struct mtr_scanner* scanner);
static struct mtr_token scan_number(struct mtr_scanner* scanner);
static struct mtr_token scan_identifier(struct mtr_scanner* scanner);
static struct mtr_token scan_comment(struct mtr_scanner* scanner);

struct mtr_token mtr_next_token(struct mtr_scanner* scanner) {
    skip_whitespace(scanner);

    scanner->start = scanner->current;

    char c_ = advance(scanner);
    char current = *scanner->current;

    switch (c_)
    {
    case '+': return make_token(scanner, MTR_TOKEN_PLUS);

    case '-':
        if (current == '>') {
            advance(scanner);
            return make_token(scanner, MTR_TOKEN_ARROW);
        }
        return make_token(scanner, MTR_TOKEN_MINUS);

    case '*': return make_token(scanner, MTR_TOKEN_STAR);

    case '/':
        if (current == '/') {
            advance(scanner);
            return make_token(scanner, MTR_TOKEN_DOUBLE_SLASH);
        }
        return make_token(scanner, MTR_TOKEN_SLASH);

    case '%': return make_token(scanner, MTR_TOKEN_PERCENT);
    case ',': return make_token(scanner, MTR_TOKEN_COMMA);
    case ':':
        if (current == '=') {
            advance(scanner);
            return make_token(scanner, MTR_TOKEN_ASSIGN);
        }
        return make_token(scanner, MTR_TOKEN_COLON);
    case ';': return make_token(scanner, MTR_TOKEN_SEMICOLON);
    case '.': return make_token(scanner, MTR_TOKEN_DOT);
    case '(': return make_token(scanner, MTR_TOKEN_PAREN_L);
    case ')': return make_token(scanner, MTR_TOKEN_PAREN_R);
    case '[': return make_token(scanner, MTR_TOKEN_SQR_L);
    case ']': return make_token(scanner, MTR_TOKEN_SQR_R);
    case '{': return make_token(scanner, MTR_TOKEN_CURLY_L);
    case '}': return make_token(scanner, MTR_TOKEN_CURLY_R);

    case '#': return scan_comment(scanner);

    case '!':
        if (current == '=') {
            advance(scanner);
            return make_token(scanner, MTR_TOKEN_BANG_EQUAL);
        }
        return make_token(scanner, MTR_TOKEN_BANG);

    case '=': return make_token(scanner, MTR_TOKEN_EQUAL);

    case '>':
        if (current == '=') {
            advance(scanner);
            return make_token(scanner, MTR_TOKEN_GREATER_EQUAL);
        }
        return make_token(scanner, MTR_TOKEN_GREATER);

    case '<':
        if (current == '=') {
            advance(scanner);
            return make_token(scanner, MTR_TOKEN_LESS_EQUAL);
        }
        return make_token(scanner, MTR_TOKEN_LESS);

    case '&':
        if (current == '&') {
            advance(scanner);
            return make_token(scanner, MTR_TOKEN_AND);
        }
        return invalid_token;

    case '|':
        if (current == '|') {
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

    case '\0':
        return make_token(scanner, MTR_TOKEN_EOF);

    }

    return make_token(scanner, MTR_TOKEN_INVALID);
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

static void skip_whitespace(struct mtr_scanner* scanner) {
    const char* c = scanner->current;
    while(*c == ' ' || *c == '\t' || *c == '\r' || *c == '\n')
        c++;
    scanner->current = c;
}

static struct mtr_token make_token(const struct mtr_scanner* scanner, enum mtr_token_type type) {
    struct mtr_token t = {
        .type = type,
        .start = scanner->start,
        .length = scanner->current - scanner->start
    };
    return t;
}

static struct mtr_token scan_string(struct mtr_scanner* scanner) {
    while (*scanner->current != '"')
        advance(scanner);
    advance(scanner); // closing "
    return make_token(scanner, MTR_TOKEN_STRING_LITERAL);
}

static struct mtr_token scan_number(struct mtr_scanner* scanner) {
    while (is_numeric(*scanner->current))
        advance(scanner);

    if (*scanner->current == '.' && is_numeric(*(scanner->current + 1))) {
        advance(scanner);
        while (is_numeric(*scanner->current))
            advance(scanner);
        return make_token(scanner, MTR_TOKEN_FLOAT_LITERAL);
    }

    return make_token(scanner, MTR_TOKEN_INT_LITERAL);
}

static bool check_keyword(const char* start, const char* end, const struct keyword_entry k) {
    return (end - start) == k.str_len && memcmp(k.str, start, k.str_len) == 0;
}

static struct mtr_token scan_identifier(struct mtr_scanner* scanner) {
    while (is_alphanumeric(*scanner->current))
        advance(scanner);

    for (int i = 0; i < KEYWORD_COUNT; ++i) {
        const struct keyword_entry k = keywords[i];
        if (check_keyword(scanner->start, scanner->current, k))
            return make_token(scanner, k.type);
    }

    return make_token(scanner, MTR_TOKEN_IDENTIFIER);
}

static struct mtr_token scan_comment(struct mtr_scanner* scanner) {
    while (*scanner->current != '\n')
        advance(scanner);
    return make_token(scanner, MTR_TOKEN_COMMENT);
}


// ================ DEBUG ===============

#ifndef NDEBUG

#include "core/log.h"

void mtr_print_token(struct mtr_token token) {
    const char* type = mtr_token_type_to_str(token.type);

    if (token.type == MTR_TOKEN_IDENTIFIER || token.type == MTR_TOKEN_STRING_LITERAL || token.type == MTR_TOKEN_INT_LITERAL ||token.type == MTR_TOKEN_FLOAT_LITERAL || token.type == MTR_TOKEN_INVALID) {
        MTR_LOG_DEBUG("Token: %s, (%.*s)", type, (u32)token.length, token.start);
    } else {
        MTR_LOG_DEBUG("Token: %s", type);
    }
}

const char* mtr_token_type_to_str(enum mtr_token_type type) {
    switch (type)
    {
    case MTR_TOKEN_PLUS:          return "+";
    case MTR_TOKEN_MINUS:         return "-";
    case MTR_TOKEN_STAR:          return "*";
    case MTR_TOKEN_SLASH:         return "/";
    case MTR_TOKEN_PERCENT:       return "%";
    case MTR_TOKEN_COMMA:         return ",";
    case MTR_TOKEN_COLON:         return ":";
    case MTR_TOKEN_SEMICOLON:     return ";";
    case MTR_TOKEN_DOT:           return ".";
    case MTR_TOKEN_PAREN_L:       return "(";
    case MTR_TOKEN_PAREN_R:       return ")";
    case MTR_TOKEN_SQR_L:         return "[";
    case MTR_TOKEN_SQR_R:         return "]";
    case MTR_TOKEN_CURLY_L:       return "{";
    case MTR_TOKEN_CURLY_R:       return "}";
    case MTR_TOKEN_BANG:          return "!";
    case MTR_TOKEN_ASSIGN:        return ":=";
    case MTR_TOKEN_GREATER:       return ">";
    case MTR_TOKEN_LESS:          return "<";
    case MTR_TOKEN_ARROW:         return "->";
    case MTR_TOKEN_BANG_EQUAL:    return "!=";
    case MTR_TOKEN_EQUAL:         return "=";
    case MTR_TOKEN_GREATER_EQUAL: return ">=";
    case MTR_TOKEN_LESS_EQUAL:    return "<=";
    case MTR_TOKEN_DOUBLE_SLASH:  return "//";
    case MTR_TOKEN_STRING_LITERAL:return "STRING";
    case MTR_TOKEN_INT_LITERAL:   return "INT";
    case MTR_TOKEN_FLOAT_LITERAL: return "FLOAT";
    case MTR_TOKEN_AND:           return "&&";
    case MTR_TOKEN_OR:            return "||";
    case MTR_TOKEN_STRUCT:        return "struct";
    case MTR_TOKEN_IF:            return "if";
    case MTR_TOKEN_ELSE:          return "else";
    case MTR_TOKEN_TRUE:          return "true";
    case MTR_TOKEN_FALSE:         return "false";
    case MTR_TOKEN_FN:            return "fn";
    case MTR_TOKEN_RETURN:        return "return";
    case MTR_TOKEN_WHILE:         return "while";
    case MTR_TOKEN_FOR:           return "for";
    case MTR_TOKEN_INT:           return "Int";
    case MTR_TOKEN_FLOAT:         return "Float";
    case MTR_TOKEN_BOOL:          return "Bool";
    case MTR_TOKEN_IDENTIFIER:    return "IDENTIFIER";
    case MTR_TOKEN_COMMENT:       return "comment";
    case MTR_TOKEN_EOF:           return "EOF";
    case MTR_TOKEN_INVALID:       return "invalid";
    }
    return "invalid";
}

#endif
