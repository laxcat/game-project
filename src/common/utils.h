#pragma once
#include "types.h"
#include <stdio.h>


inline void print16f(float const * f) {
    printf(
        "%0.4f %0.4f %0.4f %0.4f\n"
        "%0.4f %0.4f %0.4f %0.4f\n"
        "%0.4f %0.4f %0.4f %0.4f\n"
        "%0.4f %0.4f %0.4f %0.4f\n\n", 
        f[ 0], f[ 1], f[ 2], f[ 3],
        f[ 4], f[ 5], f[ 6], f[ 7],
        f[ 8], f[ 9], f[10], f[11],
        f[12], f[13], f[14], f[15]
    );
}