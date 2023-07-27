// #pragma once
// #include <assert.h>
// #include <stdarg.h>
// #include <stdio.h>
// #include "Array.h"
// #include "View.h"

// /*
// Designed to be used within pre-allocated memory, like inside a MemMan block.
// Expects `_size` bytes of pre-allocated (safe) memory directly after its own instance.
// */

// class Pool {
// public:
//     friend class MemMan;

//     size_t size() const { return _objMaxCount * _objSize; }
//     size_t objMaxCount() const { return _objMaxCount; }
//     size_t objSize() const { return _objSize; }
//     size_t freeIndex() const { return _freeIndex; }
//     byte_t * data() const { return (byte_t *)this + sizeof(Pool); }

//     // reserve up to (but not including) index from current head and return array.
//     // doesn't write anything into the space. (TODO: zero init space?)
//     template <typename T, typename ... TP>
//     Array<T> reserveTo(size_t index, TP && ... params) {
//         // checks
//         if (sizeof(T) != _objSize) {
//             fprintf(stderr, "Could not claim, size of T (%zu) does not match _objSize (%zu).",
//                 sizeof(T), _objSize);
//             assert(false);
//         }
//         if (index >= _objMaxCount) {
//             fprintf(stderr, "Could not claim, index (%zu) out of range (max: %zu).",
//                 index, _objMaxCount);
//             return Array<T>{};
//         }
//         if (index <= _freeIndex) {
//             fprintf(stderr, "Could not claim, freeIndex (%zu) already greater than requested index (%zu).",
//                 _freeIndex, index);
//             return Array<T>{};
//         }

//         // return start of array, at current head
//         T * ptr = at<T>(_freeIndex);

//         // calc count and move head
//         size_t reserveCount = index - _freeIndex;
//         _freeIndex = index;

//         return Array{ptr, reserveCount};
//     }

//     // claim up to (but not including) index from current head and return view
//     // do a placement new in each newly claimed slot
//     template <typename T, typename ... TP>
//     View<T> claimTo(size_t index, TP && ... params) {
//         Array<T> a = reserveTo<T>(index);
//         if (!a.isValid()) return View<T>{};
//         while (a.count() < a.max()) {
//             new (a.claim()) T{static_cast<TP&&>(params)...};
//         }
//         return View<T>{&a[0], a.max()};
//     }

//     // claim count number of objects and return start of array
//     template <typename T, typename ... TP>
//     View<T> claim(size_t count = 1, TP && ... params) {
//         return claimTo<T>(_freeIndex + count, static_cast<TP&&>(params)...);
//     }

//     template <typename T>
//     T * at(size_t index) {
//         assert(index < _objMaxCount && "Index out of range.");
//         return (T *)(data() + _objSize * index);
//     }
//     template <typename T> T * at() { return at<T>(_freeIndex); }

//     void reset() { _freeIndex = 0; }

// private:
//     size_t _objMaxCount = 0;
//     size_t _objSize = 0;
//     size_t _freeIndex = 0;

//     Pool(size_t objCount, size_t objSize) :
//         _objMaxCount(objCount),
//         _objSize(objSize)
//     {
//     }
// };
