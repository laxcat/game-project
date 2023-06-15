#pragma once
#include "../common/types.h"

// returns padding necessary to add to size for it to be aligned
inline size_t alignPadding(size_t size, size_t align) {
    if (align == 0) return 0;
    size_t remainder = size % align;
    return (remainder) ? align - remainder : 0;
}

// returns ptr moved forward to next alignment point
inline void * alignPtr(void * ptr, size_t align) {
    return (void *)((byte_t *)ptr + alignPadding((size_t)ptr, align));
}

// returns data size increased to alignment padding
inline size_t alignSize(size_t size, size_t align) {
    return size + alignPadding(size, align);
}
