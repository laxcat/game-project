#pragma once
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

#if DEBUG
#include <stdio.h>
#endif

class Number {
public:
    Number(char const * str, uint32_t length = UINT32_MAX) {
        // special case, all invalid
        if (length == 0) {
            return;
        }

        set(str, length, TYPE_UINT);
        set(str, length, TYPE_INT);
        set(str, length, TYPE_DOUBLE);
    }

    operator uint64_t()      const { validate(TYPE_UINT); return _uint; }
    operator uint32_t()      const { validate(TYPE_UINT, 0, UINT32_MAX); return (uint32_t)_uint; }
    operator unsigned long() const { validate(TYPE_UINT, 0, ULONG_MAX); return (unsigned long)_uint; }
    operator bool()          const { validate(TYPE_UINT); return (bool)_uint; }
    operator int64_t()       const { validate(TYPE_INT); return _int; }
    operator long()          const { validate(TYPE_INT, LONG_MIN, LONG_MAX); return (long)_int; }
    operator int()           const { validate(TYPE_INT, INT_MIN, INT_MAX); return (int)_int; }
    operator double()        const { validate(TYPE_DOUBLE); return _double; }
    operator float()         const { validate(TYPE_DOUBLE); return (float)_double; }

private:
    enum Type {
        TYPE_NONE      = 0,
        TYPE_UINT      = 1 << 0,
        TYPE_INT       = 1 << 1,
        TYPE_DOUBLE    = 1 << 2
    };

    uint64_t _uint = 0;
    int64_t _int = 0;
    double _double = 0;
    Type _valid = TYPE_NONE;

    void set(char const * str, uint32_t length, Type type) {
        if (type == TYPE_NONE) {
            return;
        }

        char * end = NULL;
        errno = 0;
        switch (type) {
        case TYPE_UINT:     { _uint     = strtoull(str, &end, 10);  break; }
        case TYPE_INT:      { _int      = strtoll(str, &end, 10);   break; }
        case TYPE_DOUBLE:   { _double   = strtod(str, &end);        break; }
        default: {}
        }

        if (errno == 0 && ((length < UINT32_MAX && str + length == end) || (*end == '\0'))) {
            _valid = (Type)(_valid | type);
        }
    }

    #if DEBUG
    void validate(Type type, int64_t min = 0, uint64_t max = 0) const {
        if ((_valid & type) == 0) {
            fprintf(stderr, "Converting to invalid type. (_valid:%d, type:%d)\n", _valid, type);
        }
        switch (type) {
        case TYPE_UINT: {
            if (max && _uint > max) fprintf(stderr, "Uint out of range.\n");
            else if ((_valid & TYPE_INT) && _int < 0) fprintf(stderr, "Negative value (%lld) cast as uint.\n", _int);
            break;
        }
        case TYPE_INT: {
            if (min && _int < min) fprintf(stderr, "Int out of min range.\n");
            else if (max && _int > (int64_t)max) fprintf(stderr, "Int out of max range.\n");
            break;
        }
        case TYPE_DOUBLE: {}
        default: {}
        }
    }
    #else
    void validate(Type, int64_t = 0, uint64_t = 0) const {}
    #endif // DEBUG
};
