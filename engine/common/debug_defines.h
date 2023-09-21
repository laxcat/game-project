#pragma once

#ifndef NDEBUG
    #define DEBUG 1
    // #define NDEBUG 0
    inline void debugBreak() {}
    inline void debugBreakEditor() {}
#else
    #define DEBUG 0
    #define NDEBUG 1
#endif
