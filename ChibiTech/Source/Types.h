#pragma once

#include <cstdint>

// type defines
//
using c8  = char8_t;
using c16 = char16_t;
using c32 = char32_t;

using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

using uptr = uintptr_t;
using iptr = intptr_t;

using b8  = uint8_t;
using b16 = uint16_t;
using b32 = uint32_t;
using b64 = uint64_t;

union u128
{
    struct { s64 Upper, Lower; };
    s64 Bits[2];
};

template<typename T> constexpr T _KB(T V) { return V * 1024; }
template<typename T> constexpr T _MB(T V) { return _KB(V) * 1024; }
template<typename T> constexpr T _GB(T V) { return _MB(V) * 1024; }

constexpr s64 _64KB  = _KB(64);
constexpr s64 _1MB   = _MB(1);
constexpr s64 _2MB   = _MB(2);
constexpr s64 _4MB   = _MB(4);
constexpr s64 _8MB   = _MB(8);
constexpr s64 _16MB  = _MB(16);
constexpr s64 _32MB  = _MB(32);
constexpr s64 _64MB  = _MB(64);
constexpr s64 _128MB = _MB(128);
constexpr s64 _256MB = _MB(256);
constexpr s64 _512MB = _MB(512);
constexpr s64 _1GB   = _GB(1);

#define file_internal static
#define var_global    static
#define var_persist   static

#define ForRange(Type, VarName, Count)        for (Type VarName = 0;           VarName < (Count); ++VarName)
#define ForRangeReverse(Type, VarName, Count) for (Type VarName = (Count) - 1; VarName >= 0;      --VarName)

#define ArrayCount(array)    (sizeof(array) / sizeof(array[0]))