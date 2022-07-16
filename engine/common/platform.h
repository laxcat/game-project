#pragma once

// Supported platforms

#define PLATFORM_MAC 0
#define PLATFORM_LINUX 0
// #define PLATFORM_WINDOWS 0
// #define PLATFORM_LINUX_WAYLAND 0
// #define PLATFORM_LINUX_X11 0

// #if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
//     #undef PLATFORM_WINDOWS
//     #define PLATFORM_WINDOWS 1
#if __APPLE__
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC
        #undef PLATFORM_MAC
        #define PLATFORM_MAC 1
    #else
        #error "Unsupported Apple platform"
    #endif
#elif __linux__
    #undef PLATFORM_LINUX
    #define PLATFORM_LINUX 1
    // #if WAYLAND_DISPLAY
    //     #define PLATFORM_LINUX_WAYLAND 1
    // #elif DISPLAY
    //    #define PLATFORM_LINUX_X11 1
    // #else
    //     #error "Unsupported linux display"
    // #endif
#else
    #error "Unsupported platform"
#endif


void * getGLContext();
