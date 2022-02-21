#ifndef _MTR_MEMORY_H
#define _MTR_MEMORY_H

#include "types.h"

void* mtr_resize_array(void* ptr, size_t old_cap, size_t new_cap, size_t elem_size);

#endif
