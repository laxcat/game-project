#pragma once

#ifndef NDEBUG
    #define DEBUG 1
    inline void debugBreak() {}
#else
    #define DEBUG 0
#endif
