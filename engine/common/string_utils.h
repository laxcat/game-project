#pragma once
#include <stdio.h>
#include "types.h"

char const * relPath(char const * path);

void setName(char * dest, char const * name, int size = 32);

bool strEqu(char const * strA, char const * strB);

bool strEqualForLength(char const * strA, char const * strB, size_t length);

// finds if str exactly matches a sub string in a comma seperated strGroup
// eg:
// strWithin("hello", "hello,goodbye") // return true
// strWithin("hell", "hello,goodbye") // return false
bool strWithin(char const * str, char const * strGroup, char sep = ',');

struct ImageData {
    byte_t * data;
    int width = 0;
    int height = 0;
    int nChannels = 0;
    size_t dataSize() const { return width * height * nChannels; };
};
ImageData loadImageBase64(char const * data, size_t length);
