#include "scanner.h"

#include "base/log.h"

static const struct mtr_token mtr_invalid_token = {
    .type = MTR_TOKEN_INVALID,
    .start = NULL,
    .length = 0
};

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
        .length = scanner->current - scanner->start,
        .type = type
    };
    return t;
}

struct mtr_token next_token(struct mtr_scanner* scanner) {
    scanner->current = skip_whitespace(scanner->current);
    scanner->start = scanner->current;

    char c = advance(scanner);

    switch (c)
    {
    case '+': return make_token(scanner, MTR_TOKEN_PLUS);

    case '-':
        if (peek(scanner->current) == '>') {
            ++scanner->current;
            return make_token(scanner, MTR_TOKEN_ARROW);
        }
        return make_token(scanner, MTR_TOKEN_MINUS);

    case '*': return make_token(scanner, MTR_TOKEN_STAR);
    case '/': return make_token(scanner, MTR_TOKEN_SLASH);
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
            ++scanner->current;
            return make_token(scanner, MTR_TOKEN_BANG_EQUAL);
        }
        return make_token(scanner, MTR_TOKEN_BANG);

    case '=':
        if (peek(scanner->current) == '=') {
            ++scanner->current;
            return make_token(scanner, MTR_TOKEN_EQUAL_EQUAL);
        }
        return make_token(scanner, MTR_TOKEN_EQUAL);

    case '>':
        if (peek(scanner->current) == '=') {
            ++scanner->current;
            return make_token(scanner, MTR_TOKEN_GREATER_EQUAL);
        }
        return make_token(scanner, MTR_TOKEN_GREATER);

    case '<':
        if (peek(scanner->current) == '=') {
            ++scanner->current;
            return make_token(scanner, MTR_TOKEN_LESS_EQUAL);
        }
        return make_token(scanner, MTR_TOKEN_LESS);

    default:
        break;
    }

    return mtr_invalid_token;
}
