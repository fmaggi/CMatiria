#include "memory.h"

#include "log.h"

#include <stdlib.h>
#include <string.h>

void* mtr_resize_array(void* ptr, size_t old_cap, size_t new_cap, size_t elem_size) {
    void* temp = malloc(elem_size * new_cap);
    if (NULL == temp) {
        free(ptr);
        free(temp);
        MTR_LOG_ERROR("Bad allocation.");
        return NULL;
    }

    if (new_cap > old_cap && 0 != old_cap) {
        memcpy(temp, ptr, elem_size * old_cap);
    }

    free(ptr);
    return temp;
}
