#include "print.h"
#include <stdio.h>
#if DEV_INTERFACE
#include "../MrManager.h"
#endif

void vprint(char const * formatString, va_list args) {
    vfprintf(stdout, formatString, args);
    #if DEV_INTERFACE
    mm.devOverlay.vprint(formatString, args);
    #endif
}
