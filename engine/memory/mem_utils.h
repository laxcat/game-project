#pragma once
#include "../common/types.h"

inline char const * memBlockTypeStr(MemBlockType type) {
    switch (type) {
    case MEM_BLOCK_FREE:       return "FREE";
    case MEM_BLOCK_REQUEST:    return "REQUEST";
    case MEM_BLOCK_FSA:        return "FSA";
    case MEM_BLOCK_CLAIMED:    return "CLAIMED";
    case MEM_BLOCK_ARRAY:      return "ARRAY";
    case MEM_BLOCK_CHARKEYS:   return "CHARKEYS";
    case MEM_BLOCK_FILE:       return "FILE";
    case MEM_BLOCK_FRAMESTACK: return "FRAMESTACK";
    case MEM_BLOCK_FREELIST:   return "FREELIST";
    case MEM_BLOCK_GLTF:       return "GLTF";
    case MEM_BLOCK_GOBJ:       return "GOBJ";
    case MEM_BLOCK_POOL:       return "POOL";
    case MEM_BLOCK_BGFX:       return "BGFX";
    case MEM_BLOCK_GENERIC:    return "GENERIC";
    case MEM_BLOCK_FILTER_ALL: return "ALL";
    default:                   return "Unknown Type";
    }
}

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
