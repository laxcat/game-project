#pragma once
#include <stdarg.h>

/*

Series of print functions designed to be slightly more conveient than standard
stdio.h functions. Also allows interception of print calls in order to display
stdout within the runtime, in something like a dev overlay (which is disabled atm).

*/

void vprint(char const * formatString, va_list args);

inline void print(char const * formatString, ...) {
    va_list args;
    va_start(args, formatString);
    vprint(formatString, args);
    va_end(args);
}

inline void printl(char const * formatString, ...) {
    va_list args;
    va_start(args, formatString);
    vprint(formatString, args);
    vprint("\n", nullptr);
    va_end(args);
}
inline void printl() {
    vprint("\n", nullptr);
}
inline void vprintl(char const * formatString, va_list args) {
    vprint(formatString, args);
    vprint("\n", nullptr);
}

inline void printc(bool shouldPrint, char const * formatString, ...) {
    if (!shouldPrint) return;
    va_list args;
    va_start(args, formatString);
    vprint(formatString, args);
    va_end(args);
}

// UNTESTED
template <bool SHOULD_PRINT>
inline void printc(char const * formatString, ...) {
    if constexpr (!SHOULD_PRINT) return;
    va_list args;
    va_start(args, formatString);
    vprint(formatString, args);
    va_end(args);
}

inline void print4f(float const * f) {
    print(
        "%0.4f %0.4f %0.4f %0.4f\n",
        f[ 0], f[ 1], f[ 2], f[ 3]
    );
}

inline void print16f(float const * f, char const * indent) {
    print(
        "%s%0.4f %0.4f %0.4f %0.4f\n"
        "%s%0.4f %0.4f %0.4f %0.4f\n"
        "%s%0.4f %0.4f %0.4f %0.4f\n"
        "%s%0.4f %0.4f %0.4f %0.4f\n", 
        indent, f[ 0], f[ 1], f[ 2], f[ 3],
        indent, f[ 4], f[ 5], f[ 6], f[ 7],
        indent, f[ 8], f[ 9], f[10], f[11],
        indent, f[12], f[13], f[14], f[15]
    );
}
inline void print16fc(bool shouldPrint, float const * f, char const * indent) {

    if (shouldPrint) print16f(f, indent);
}
