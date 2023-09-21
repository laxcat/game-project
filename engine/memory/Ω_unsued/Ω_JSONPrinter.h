#pragma once
#include "../dev/print.h"

static constexpr char const * PAD = "                            ";
static constexpr size_t MaxStrLen = 50;
#define PNUL(TYPE)              printl("%.*s" #TYPE "()", depth*4, PAD)
#define PVAL(TYPE, FORMAT, VAL) printl("%.*s" #TYPE "(" #FORMAT ")", depth*4, PAD, VAL)
#define PSTR(TYPE) \
    if (length < MaxStrLen) \
        printl("%.*s" #TYPE "(%s, %zu)", depth*4, PAD, str, length); \
    else \
        printl("%.*s" #TYPE "(%.*s..., %zu)", depth*4, PAD, MaxStrLen, str, length)

struct JSONPrinter {
    size_t depth = 0;

    bool Null()             { PNUL(  Null);        return true; }
    bool Bool(bool b)       { PVAL(  Bool, %d, b); return true; }
    bool Int(int i)         { PVAL(   Int, %d, i); return true; }
    bool Uint(unsigned u)   { PVAL(  Uint, %u, u); return true; }
    bool Int64(int64_t i)   { PVAL( Int64, %d, i); return true; }
    bool Uint64(uint64_t u) { PVAL(Uint64, %u, u); return true; }
    bool Double(double d)   { PVAL(Double, %f, d); return true; }

    bool RawNumber(char const * str, size_t length, bool copy) { PSTR(RawNumber); return true; }
    bool String   (char const * str, size_t length, bool copy) { PSTR(String);    return true; }
    bool Key      (char const * str, size_t length, bool copy) { PSTR(Key);       return true; }

    bool StartObject() { PNUL(StartObject); ++depth; return true; }
    bool StartArray () { PNUL(StartArray);  ++depth; return true; }
    bool EndObject(size_t  memberCount) { --depth; PVAL(EndObject, %zu,  memberCount); return true; }
    bool EndArray (size_t elementCount) { --depth; PVAL(EndObject, %zu, elementCount); return true; }
};
