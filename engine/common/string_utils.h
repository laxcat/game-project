#pragma once
#include <stdio.h>
#include "types.h"

void setName(char * dest, char const * name, int size = 32);

// a more convenient strcmp, privided you only need to know equal or not
bool strEqu(char const * strA, char const * strB);
// faster but requires length. compares in 8-byte chunks
bool strEqu(char const * strA, char const * strB, size_t length);

// finds if str exactly matches a sub string in a comma seperated strGroup
// eg:
// strWithin("hello", "hello,goodbye") // return true
// strWithin("hell", "hello,goodbye") // return false
bool strWithin(char const * str, char const * strGroup, char sep = ',');

// https://stackoverflow.com/a/14530993
void urldecode2(char * dst, char const * src);
int urldecode2_length(char const * src);

// struct ImageData {
//     byte_t * data;
//     int width = 0;
//     int height = 0;
//     int nChannels = 0;
//     size_t dataSize() const { return width * height * nChannels; };
// };
// ImageData loadImageBase64(char const * data, size_t length);

template <size_t MAX>
void fixstrcpy(char * dst, char const * src) {
    // if even multiple of 8 bytes, copy as 8-byte chunks
    // TODO: this could be changed to copy chunks for all strings longer than 8
    // bytes, even if not a multiple of 8. (do as many chucks as possible, then
    // fill in the rest, etc.)
    if constexpr (MAX % 8 == 0) {
        constexpr size_t N_CHUNKS = MAX / 8;
        for (size_t i = 0; i < N_CHUNKS; ++i) {
            ((uint64_t *)dst)[i] = ((uint64_t *)src)[i];
        }
    }
    // otherwise copy byte by byte, potentially returning early
    else {
        for (size_t i = 0; i < MAX; ++i) {
            dst[i] = src[i];
            if (src[i] == '\0') {
                return;
            }
        }
    }
    // place final null byte
    dst[MAX-1] = '\0';
}
