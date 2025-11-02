#ifndef BITS_HPP
#define BITS_HPP

#include <cstdint>

// Im on ubuntu and was afraid of using builtins, hence here are software solutions

template<typename T>
uint64_t clz(T x) {
  // edge case for WebAssembly
    if (x == 0)
        return sizeof(T) * 8;
    uint64_t n = 0;
    T bit = T(1) << (sizeof(T) * 8 - 1);
    while (!(x & bit)) {
        n++;
        bit >>= 1;
    }
    return n;
}

template<typename T>
uint64_t ctz(T x) {
    if (x == 0)
        return sizeof(T) * 8;
    uint64_t n = 0;
    while ((x & 1) == 0) {
        n++;
        x >>= 1;
    }
    return n;
}

template<typename T>
uint64_t popcnt(T x) {
    uint64_t n = 0;
    while (x) {
        n += x & 1;
        x >>= 1;
    }
    return n;
}

#endif // BITS_HPP
