#pragma once
#include <stdio.h>
#include <filesystem>

inline char const * relPath(char const * path) {
    auto rel = std::filesystem::relative(path);
    static char ret[1024];
    snprintf(ret, 1024, "%s", rel.c_str());
    return ret;
}

inline void setName(char * dest, char const * name, size_t size = 32) {
    if (size == 0) return;
    snprintf(dest, size, "%s", name);
}
