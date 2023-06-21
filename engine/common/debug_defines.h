#pragma once

#ifndef NDEBUG
    #define DEBUG 1
    // #define NDEBUG 0
    inline void debugBreak() {}
#else
    #define DEBUG 0
    #define NDEBUG 1
#endif
