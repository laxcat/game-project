#include "string_utils.h"
#include <stb_image.h>
#include "../MrManager.h"
#include "modp_b64.h"

void setName(char * dest, char const * name, int size) {
    if (size == 0) return;
    snprintf(dest, size, "%s", name);
}

bool strEqu(char const * strA, char const * strB, size_t size) {
    if (size) {
        return (strncmp(strA, strB, size) == 0);
    }
    return (strcmp(strA, strB) == 0);
}

// TODO: remove this, use strEqu with optional size
bool strEqualForLength(char const * strA, char const * strB, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        // one (but not both) is null or chars don't match
        if (!strA[i] != !strB[i] || strA[i] != strB[i]) return false;
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

// SOME STRING TESTS
// printl("test %d", strWithin("hello", "hello,goodbye"));
// printl("test %d", strWithin("goodbye", "hello,goodbye"));
// printl("test %d", strWithin("b", "a,b,c"));
// printl("test %d", strWithin("", ",b,c"));
// printl("test %d", strWithin("", "a,,c"));
// printl("test %d", strWithin("", "a,b,"));
// printl("test %d", strWithin("hell", "hello,goodbye"));

ImageData loadImageBase64(char const * data, size_t length) {
    if (strEqualForLength("data:image/jpeg;base64,", data, 23)) {
        data   += 23;
        length -= 23;
    }
    if (strEqualForLength("data:image/png;base64,", data, 22)) {
        data   += 22;
        length -= 22;
    }

    size_t decodedLength = modp_b64_decode_len(length);
    // printl("length %zu", length);
    // printl("modp_b64_decode_len %zu", decodedLength);

    byte_t * decodedData = (byte_t *)malloc(decodedLength);
    modp_b64_decode((char *)decodedData, data, length);

    ImageData id;
    byte_t * imgData = stbi_load_from_memory(decodedData, decodedLength, &id.width, &id.height, &id.nChannels, 0);
    if (!imgData) return id;

    id.data = mm.frameStack->alloc(id.dataSize());
    if (!id.data) {
        fprintf(stderr, "Frame stack alloc failed.\n");
        id = {};
        stbi_image_free(imgData);
        return id;
    }
    memcpy(id.data, imgData, id.dataSize());

    stbi_image_free(imgData);

    return id;
}
