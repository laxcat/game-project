#pragma once
#include "../common/types.h"

inline char const * memBlockTypeStr(MemBlockType type) {
    switch (type) {
    case MEM_BLOCK_FREE:       return "FREE";
    case MEM_BLOCK_CLAIMED:    return "CLAIMED";
    case MEM_BLOCK_FSA:        return "FSA";
    case MEM_BLOCK_POOL:       return "POOL";
    case MEM_BLOCK_STACK:      return "STACK";
    case MEM_BLOCK_FILE:       return "FILE";
    case MEM_BLOCK_GOBJ:       return "GOBJ";
    case MEM_BLOCK_BGFX:       return "BGFX";
    case MEM_BLOCK_EXTERNAL:   return "EXTERNAL";
    default:                   return "Unknown Type";
    }
}
