#include "print.h"
#include <stdio.h>
#include "../MrManager.h"


void vprint(char const * formatString, va_list args) {
    vfprintf(stderr, formatString, args);
    #if DEBUG_INTERFACE
    mm.debugOverlay.vprint(formatString, args);
    #endif // DEBUG_INTERFACE
}

void print(char const * formatString, ...) {
    va_list args;
    va_start(args, formatString);
    vprint(formatString, args);
    va_end(args);
}

void printc(bool shouldPrint, char const * formatString, ...) {
    if (!shouldPrint) return;
    va_list args;
    va_start(args, formatString);
    vprint(formatString, args);
    va_end(args);
}

void print16f(float const * f, char const * indent) {
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
void print16fc(bool shouldPrint, float const * f, char const * indent) {
    if (shouldPrint) print16f(f, indent);
}
