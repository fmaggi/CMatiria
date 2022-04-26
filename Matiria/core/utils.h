#ifndef MTR_UTILS_H
#define MTR_UTILS_H

#include "types.h"

static inline u32 hash(const char* key, size_t length) {
    u32 hash = 2166136261u;
    for (size_t i = 0; i < length; i++) {
        hash ^= (u8)key[i];
        hash *= 16777619;
    }
    return hash;
}

// got it from http://web.archive.org/web/20071223173210/http:/www.concentric.net/~Ttwang/tech/inthash.htm
static inline u32 hashi64(i64 key) {
    key = (~key) + (key << 18);
    key = key ^ (key >> 31);
    key = key * 21;
    key = key ^ (key >> 11);
    key = key + (key << 6);
    key = key ^ (key >> 22);
    return (u32) key;
}


#endif
