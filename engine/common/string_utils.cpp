#include "string_utils.h"
// #include <stb_image.h>
#include "../MrManager.h"
#include "modp_b64.h"

void setName(char * dest, char const * name, int size) {
    if (size == 0) return;
    snprintf(dest, size, "%s", name);
}

bool strEqu(char const * strA, char const * strB) {
    return (strcmp(strA, strB) == 0);
}

bool strEqu(char const * strA, char const * strB, size_t length) {
    // compare the string as 8 byte chunks
    char * a = (char *)strA;
    char * b = (char *)strB;
    while (length >= 8) {
        if (*(uint64_t *)a != *(uint64_t *)b) return false;
        a += 8;
        b += 8;
        length -= 8;
    }
    // finish off the last few bytes
    for (size_t i = 0; i < length; ++i) {
        // one (but not both) is null or chars don't match
        if (a[i] != b[i]) return false;
    }
    return true;
}

bool strWithin(char const * str, char const * strGroup, char sep) {
    assert(str && strGroup);

    // incrementing pointers
    char * sp = (char *)str;
    char * gp = (char *)strGroup;

    // while there are characters in the group pointer
    // each pass of this outer loop looks at one substr in the group
    while(*gp) {
        // special check for empty string
        if (!*sp && *gp == sep) return true;

        // iterate through this "substr"
        while(*sp) {
            if (*sp != *gp) break;
            ++sp, ++gp;
            if (!*sp && (!*gp || *gp == sep)) return true;
        }

        // move to next substr
        while(*gp && *gp != sep) ++gp;
        if (*gp == sep) ++gp;
        sp = (char *)str;

        // special check for empty string at end of group
        if (!*sp && !*gp) return true;
    }
    return false;
}

// https://stackoverflow.com/a/14530993
void urldecode2(char * dst, char const * src) {
    char a, b;
    int extra = 0;
    // fill the remaining dst buffer with \0 if same as src
    bool fillDstBuffer = (dst == src);
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b)))
        {
            if (a >= 'a')
                a -= 'a'-'A';
            if (a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';
            if (b >= 'a')
                b -= 'a'-'A';
            if (b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';
            *dst++ = 16*a+b;
            src += 3;
            extra += 2;
        }
        else if (*src == '+') {
            *dst++ = ' ';
            src++;
        }
        else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
    if (fillDstBuffer) {
        while(extra) {
            *dst++ = '\0';
            --extra;
        }
    }
}
int urldecode2_length(char const * src) {
    int length = 0;
    char a, b;
    while (*src) {
        if (*src == '%' && isxdigit(src[1]) && isxdigit(src[2])) {
            ++length;
            src += 3;
        }
        else {
            ++length;
            ++src;
        }
    }
    return length;
}

// ImageData loadImageBase64(char const * data, size_t length) {
//     if (strEqu("data:image/jpeg;base64,", data, 23)) {
//         data   += 23;
//         length -= 23;
//     }
//     if (strEqu("data:image/png;base64,", data, 22)) {
//         data   += 22;
//         length -= 22;
//     }

//     size_t decodedLength = modp_b64_decode_len(length);
//     // printl("length %zu", length);
//     // printl("modp_b64_decode_len %zu", decodedLength);

//     byte_t * decodedData = (byte_t *)malloc(decodedLength);
//     modp_b64_decode((char *)decodedData, data, length);

//     ImageData id;
//     byte_t * imgData = stbi_load_from_memory(decodedData, decodedLength, &id.width, &id.height, &id.nChannels, 0);
//     if (!imgData) return id;

//     id.data = mm.frameStack->alloc(id.dataSize());
//     if (!id.data) {
//         fprintf(stderr, "Frame stack alloc failed.\n");
//         id = {};
//         stbi_image_free(imgData);
//         return id;
//     }
//     memcpy(id.data, imgData, id.dataSize());

//     stbi_image_free(imgData);

//     return id;
// }
