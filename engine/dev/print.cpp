#include "print.h"
#include <stdio.h>


#if DEV_INTERFACE
#include "../MrManager.h"
void vprint(char const * formatString, va_list args) {
    vfprintf(stderr, formatString, args);
    mm.devOverlay.vprint(formatString, args);
}
#else
void vprint(char const * formatString, va_list args) {
    vfprintf(stderr, formatString, args);
}
#endif // DEV_INTERFACE
