#pragma once
#include "../common/types.h"
#include <stdio.h>

/*
Designed to be used within pre-allocated memory.
Expects `_size` bytes of pre-allocated (safe) memory directly after its own instance.
*/

// read only for now
class File {
public:
    friend class MemMan;

    class Path {
    public:
        static constexpr size_t Max = 1024;
        Path();
        Path(char const * str);
        Path & operator=(char const * str);
        void set(char const * str);
        char full[Max];
        char const * filename;
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
    Path _path = "";
    bool _loaded = false;

    File(size_t size, char const * path) :
        _size(size),
        _path(path)
    {}

    #if DEV_INTERFACE
    static void editorCreate();
    void editorEditBlock();
    #endif // DEV_INTERFACE
};
