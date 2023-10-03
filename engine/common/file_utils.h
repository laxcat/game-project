#pragma once
#include <stdio.h>

inline long getFileSize(FILE * fp) {
    int fseekError = fseek(fp, 0L, SEEK_END);
    if (fseekError) {
        fprintf(stderr, "Error seeking: %d\n", fseekError);
        return -1;
    }
    long fileSize = ftell(fp);
    if (fileSize == -1) {
        fprintf(stderr, "Error reading seek position: %d\n", ferror(fp));
    }
    fseek(fp, 0L, SEEK_SET);
    return fileSize;
}
inline long getFileSize(char const * path) {
    FILE * fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Error openning file: \"%s\"\n", path);
        return -1;
    }
    long ret = getFileSize(fp);
    fclose(fp);
    return ret;
}

// buf should have at least strlen(file) bytes available
inline int copyDirName(char * buf, char const * file) {
    int i = 0;
    int lastSlash = 0;
    while (file[i]) {
        if (file[i] == '/') lastSlash = i;
        ++i;
    }
    return snprintf(buf, lastSlash + 2, "%.*s", lastSlash + 1, file);
}
