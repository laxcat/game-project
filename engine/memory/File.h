#pragma once
#include "../common/types.h"
#include <stdio.h>

/*

Designed to be used within pre-allocated memory.
Expects `_size` bytes of pre-allocated (safe) memory directly after its own instance.

Read only for now

*/

class File {
private:
    friend class MemMan;
    File(size_t size, char const * path, size_t align = 0) :
        _size(size),
        _path(path),
        _align(align)
    {}

public:
    class Path {
    public:
        static constexpr size_t MAX = 256;

        char full[MAX];
        char const * filename;

        Path();
        Path(char const * str);
        Path & operator=(char const * str);
        void set(char const * str);
        bool isSet() const;
    };

    // data size is one bigger than actual file size. 0x00 byte written at end.
    size_t size() const;
    size_t head() const;
    size_t fileSize() const;
    byte_t * data() const;
    byte_t * dataHead() const;
    bool loaded() const;
    Path const & path() const;

    bool load(FILE * externalFP = nullptr);

private:
    size_t _size = 0;
    size_t _head = 0;
    size_t _align = 0; // ensure data() (aka actual file contents) are aligned to this
    Path _path = "";
    bool _loaded = false;

    #if DEV_INTERFACE
    static void editorCreate();
    void editorEditBlock();
    #endif // DEV_INTERFACE
};
