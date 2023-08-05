#include "File.h"
#include <stdarg.h>
#include <errno.h>
#include <string.h>

File::Path::Path() {
    full[0] = '\0';
    filename = full;
}

File::Path::Path(char const * str) {
    set(str);
}

File::Path & File::Path::operator=(char const * str) {
    set(str);
    return *this;
}

void File::Path::set(char const * str) {
    snprintf(full, MAX, "%s", str);
    // filename points to char after last slash. if no slash found, points to full
    size_t len = strlen(full);
    while (len > 0 && full[len-1] != '/') --len;
    filename = ((char const *)full) + len;
}

bool File::Path::isSet() const {
    return (full[0] != '\0');
}

size_t File::size() const { return _size; }
size_t File::head() const { return _head; }
size_t File::fileSize() const { return _size - 1; }
byte_t * File::data() const { return (byte_t *)this + sizeof(File); }
byte_t * File::dataHead() const { return data() + _head; }
bool File::loaded() const { return _loaded; }
File::Path const & File::path() const { return _path; }

bool File::load(FILE * externalFP) {
    FILE * fp;

    // if file was already open, user might have passed in a file pointer
    // to save from reopening it
    if (externalFP) {
        fp = externalFP;
        int fseekError = fseek(fp, 0L, 0);
        if (fseekError) {
            fprintf(stderr, "Error seeking to begining of file \"%s\" for load: %d\n", _path.full, fseekError);
            return false;
        }
    }
    else {
        errno = 0;
        fp = fopen(_path.full, "r");
        if (!fp) {
            fprintf(stderr, "Error opening file \"%s\" for loading: %d\n", _path.full, errno);
            return false;
        }
    }

    size_t readSize = fread(data(), 1, fileSize(), fp);
    if (readSize != fileSize()) {
        fprintf(stderr, "Error reading file \"%s\" contents: read %zu, expecting %zu\n",
            _path.full, readSize, fileSize());
        return false;
    }

    if (!externalFP) {
        fclose(fp);
        int fe = ferror(fp);
        if (fe) {
            fprintf(stderr, "Error closing file \"%s\" after load: %d\n", _path.full, fe);
            return false;
        }
    }

    //0x00 byte written after file contents
    data()[_size-1] = '\0';

    _loaded = true;
    return true;
}
