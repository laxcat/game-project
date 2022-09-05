class Entity {
public:
    friend class MemSys;

    // data size is one bigger than actual file size. 0x00 byte written at end.
    size_t size() const { return _size; }
    size_t head() const { return _head; }
    size_t fileSize() const { return _size - 1; }
    byte_t * data() const { return (byte_t *)this + sizeof(File); }
    byte_t * dataHead() const { return data() + _head; }
    bool loaded() const { return _loaded; }
    char const * path() const { return _path; }

    size_t static getMemorySize(FILE * externalFP);

    bool load();
    bool readJSON(FILE * externalFP);

private:
    size_t _size = 0;
    size_t _head = 0;
    char const * _path = "";
    bool _loaded = false;

    Entity(char const * path) :
        _path(path)
    {
    }
};
