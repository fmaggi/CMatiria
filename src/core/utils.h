#ifndef _MTR_UTILS_H
#define _MTR_UTILS_H

#include "types.h"

static inline u32 hash(const char* key, size_t length) {
    u32 hash = 2166136261u;
    for (size_t i = 0; i < length; i++) {
        hash ^= (u8)key[i];
        hash *= 16777619;
    }
    return hash;
}

#endif
