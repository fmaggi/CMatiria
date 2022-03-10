#ifndef MTR_OBJECT_H
#define MTR_OBJECT_H

#include "value.h"

#include "core/types.h"

typedef void mtr_object;

struct mtr_array {
    mtr_value* elements;
    size_t size;
    size_t capacity;
};

struct mtr_array* mtr_new_array(void);
void mtr_delete_array(struct mtr_array* array);

void mtr_array_append(struct mtr_array* array, mtr_value value);
mtr_value mtr_array_pop(struct mtr_array* array);
// void mtr_array_insert(struct mtr_array* array, mtr_value value, size_t index);



#endif
