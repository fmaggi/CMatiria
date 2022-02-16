#include "token.h"

#include "core/log.h"

#include <stdlib.h>
#include <string.h>

struct mtr_token_array mtr_new_token_array() {
    struct mtr_token_array array;
    array.capacity = 8;
    array.size = 0;
    array.tokens = malloc(sizeof(struct mtr_token) * 8);
    if (NULL == array.tokens)
        MTR_LOG_ERROR("Invalid allocation!");
    return array;
}

void mtr_write_token(struct mtr_token_array* array, struct mtr_token token) {
    if (array->size == array->capacity) {
        size_t new_capacity = array->size * 2;
        struct mtr_token* temp = malloc(sizeof(struct mtr_token) * new_capacity);
        if (NULL == temp) {
            MTR_LOG_ERROR("Invalid allocation!");
            return;
        }

        memcpy(temp, array->tokens, sizeof(struct mtr_token) * array->capacity);
        free(array->tokens);
        array->tokens = temp;
        array->capacity = new_capacity;
    }

    array->tokens[array->size++] = token;
}
void mtr_delete_token_array(struct mtr_token_array* array) {
    free(array->tokens);
    array->tokens = NULL;
    array->size = 0;
    array->capacity = 0;
}

void mtr_print_token(struct mtr_token token) {
    const char* type = mtr_token_type_to_str(token.type);

    if (token.type == MTR_TOKEN_IDENTIFIER || token.type == MTR_TOKEN_STRING || token.type == MTR_TOKEN_INT ||token.type == MTR_TOKEN_FLOAT || token.type == MTR_TOKEN_INVALID) {
        MTR_LOG_TRACE("Token: %s at %i, (%.*s)", type, token.char_idx, (u32)token.length, token.start);
    } else {
        MTR_LOG_TRACE("Token: %s at %i", type, token.char_idx);
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
    case MTR_TOKEN_EQUAL:         return "=";
    case MTR_TOKEN_GREATER:       return ">";
    case MTR_TOKEN_LESS:          return "<";
    case MTR_TOKEN_ARROW:         return "->";
    case MTR_TOKEN_BANG_EQUAL:    return "!=";
    case MTR_TOKEN_EQUAL_EQUAL:   return "==";
    case MTR_TOKEN_GREATER_EQUAL: return ">=";
    case MTR_TOKEN_LESS_EQUAL:    return "<=";
    case MTR_TOKEN_DOUBLE_SLASH:  return "//";
    case MTR_TOKEN_STRING:        return "STRING";
    case MTR_TOKEN_INT:           return "INT";
    case MTR_TOKEN_FLOAT:         return "FLOAT";
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
    case MTR_TOKEN_U8:            return "u8";
    case MTR_TOKEN_U16:           return "u16";
    case MTR_TOKEN_U32:           return "u32";
    case MTR_TOKEN_U64:           return "u64";
    case MTR_TOKEN_I8:            return "i8";
    case MTR_TOKEN_I16:           return "i16";
    case MTR_TOKEN_I32:           return "i32";
    case MTR_TOKEN_I64:           return "i64";
    case MTR_TOKEN_F32:           return "f32";
    case MTR_TOKEN_F64:           return "f64";
    case MTR_TOKEN_BOOL:          return "bool";
    case MTR_TOKEN_IDENTIFIER:    return "IDENTIFIER";
    case MTR_TOKEN_NEWLINE:       return "newline";
    case MTR_TOKEN_COMMENT:       return "comment";
    case MTR_TOKEN_EOF:           return "EOF";
    case MTR_TOKEN_INVALID:       return "invalid";
    }
    return "invalid";
}
