#include "string_utils.h"
#include <stdio.h>
#include <filesystem>
#include "../MrManager.h"

char const * relPath(char const * path) {
    auto rel = std::filesystem::relative(path);
    return mm.frameStack->formatStr("%s", rel.c_str());
}

void setName(char * dest, char const * name, int size) {
    if (size == 0) return;
    snprintf(dest, size, "%s", name);
}
