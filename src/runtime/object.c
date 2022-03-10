#include "object.h"

#include "core/log.h"

#include <stdlib.h>

struct mtr_array* mtr_new_array(void) {
    struct mtr_array* a = malloc(sizeof(*a));
    if (NULL == a) {
        MTR_LOG_ERROR("Bad allocation.");
        return NULL;
    }

    void* temp = malloc(sizeof(mtr_value) * 8);
    if (NULL == temp) {
        free(a);
        MTR_LOG_ERROR("Bad allocation.");
        return NULL;
    }

    a->elements = temp;
    a->capacity = 8;
    a->size = 0;
    return a;
}

void mtr_delete_array(struct mtr_array* array) {
    free(array->elements);
    array->elements = NULL;
    free(array);
}

void mtr_array_append(struct mtr_array* array, mtr_value value) {
    if (array->size == array->capacity) {
        size_t new_cap = array->capacity * 2;
        array->elements = realloc(array->elements, new_cap * sizeof(mtr_value));
        array->capacity = new_cap;
    }

    array->elements[array->size++] = value;
}

mtr_value mtr_array_pop(struct mtr_array* array) {
    return array->elements[--array->size];
}
