// read only for now
class File {
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

    bool load(FILE * externalFP = nullptr) {
        FILE * fp;

        // if file was already open, user might have passed in a file pointer
        // to save from reopening it
        if (externalFP) {
            fp = externalFP;
            int fseekError = fseek(fp, 0L, 0);
            if (fseekError) {
                fprintf(stderr, "Error seeking to begining of file \"%s\" for load: %d\n", _path, fseekError);
                return false;
            }
        }
        else {
            errno = 0;
            fp = fopen(_path, "r");
            if (!fp) {
                fprintf(stderr, "Error opening file \"%s\" for loading: %d\n", _path, errno);
                return false;
            }
        }

        size_t readSize = fread(data(), 1, fileSize(), fp);
        if (readSize != fileSize()) {
            fprintf(stderr, "Error reading file \"%s\" contents: read %zu, expecting %zu\n",
                _path, readSize, fileSize());
            return false;
        }

        if (!externalFP) {
            fclose(fp);
            int fe = ferror(fp);
            if (fe) {
                fprintf(stderr, "Error closing file \"%s\" after load: %d\n", _path, fe);
                return false;
            }
        }

        //0x00 byte written after file contents
        data()[_size-1] = '\0';

        _loaded = true;
        return true;
    }

private:
    size_t _size = 0;
    size_t _head = 0;
    char const * _path = "";
    bool _loaded = false;

    File(size_t size, char const * path) :
        _size(size),
        _path(path)
    {
    }
};
