#include "ast_nodes.h"

#include "core/log.h"

#include <stdlib.h>
#include <string.h>

struct mtr_expr_array mtr_new_expr_array() {
    struct mtr_expr_array array = {
        .capacity = 8,
        .size = 0,
        .expressions = malloc(sizeof(struct mtr_expr) * 8)
    };
    if (NULL == array.expressions)
        MTR_LOG_ERROR("Invalid allocation!");
    return array;
}

struct mtr_expr* mtr_write_expr(struct mtr_expr_array* array, struct mtr_expr expr) {
    if (array->size == array->capacity) {
        size_t new_capacity = array->size * 2;
        struct mtr_expr* temp = malloc(sizeof(struct mtr_expr) * new_capacity);
        if (NULL == temp) {
            MTR_LOG_ERROR("Invalid allocation!");
            return NULL;
        }

        memcpy(temp, array->expressions, sizeof(struct mtr_expr) * array->capacity);
        free(array->expressions);
        array->expressions = temp;
        array->capacity = new_capacity;
    }

    array->expressions[array->size++] = expr;
    return array->expressions + array->size - 1;
}

struct mtr_expr* mtr_get_expr_ptr(struct mtr_expr_array* array, u64 index) {
    return array->expressions + index;
}

struct mtr_expr mtr_get_expr(struct mtr_expr_array* array, u64 index) {
    return array->expressions[index];
}

void mtr_delete_expr_array(struct mtr_expr_array* array) {
    free(array->expressions);
    array->expressions = NULL;
    array->size = 0;
    array->capacity = 0;
}

static void print_expr(struct mtr_expr* parser);

static void print_literal(struct mtr_literal* node) {
    printf("%.*s ", (u32)node->token.length, node->token.start);
}

static void print_unary(struct mtr_unary* node) {
    printf("%.*s", (u32)node->operator.length, node->operator.start);
    print_expr(node->right);
}

static void print_binary(struct mtr_binary* node) {
    printf("(%.*s ", (u32)node->operator.length, node->operator.start);
    print_expr(node->left);
    print_expr(node->right);
    printf(")");
}

static void print_expr(struct mtr_expr* node) {
    switch (node->type)
    {
    case MTR_EXPR_LITERAL:
        print_literal((struct mtr_literal*) node);
        break;
    case MTR_EXPR_BINARY:
        print_binary((struct mtr_binary*) node);
        break;
    case MTR_EXPR_GROUPING:
        break;
    case MTR_EXPR_UNARY:
        print_unary((struct mtr_unary*) node);
        break;
    }
}

void mtr_print_expr(struct mtr_expr* node) {
    MTR_LOG_DEBUG("Expression: ");
    print_expr(node);
    putc('\n', stdout);
}

