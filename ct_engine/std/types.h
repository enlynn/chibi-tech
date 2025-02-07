#ifndef _TYPES_H_
#define _TYPES_H_

#include <stdint.h>
#include <float.h>
#include <stddef.h>
#include <stdbool.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  define CT_PLATFORM_WIN32 1
#  define CT_PLATFORM_APPLE 0
#  define CT_PLATFORM_LINUX 0
#elif defined(__linux__)
#  define CT_PLATFORM_WIN32 0
#  define CT_PLATFORM_APPLE 0
#  define CT_PLATFORM_LINUX 1
#elif defined(__APPLE__)
#  define CT_PLATFORM_WIN32 0
#  define CT_PLATFORM_APPLE 1
#  define CT_PLATFORM_LINUX 0
#else
#  error "Target Platform is not supported."
#endif

#if defined(__aarch64__)
#  define CT_ARM64 1
#  define CT_X64   0
#elif defined(__x86_64__) || defined(_M_X64)
#  define CT_X64   1
#  define CT_ARM64 0
#else
#  error "Target architecture is not supported."
#endif

#if defined(_DEBUG) || (DEBUG)
#  define CT_DEBUG_BUILD  1
#  define CT_PUBLIC_BUILD 0
#else
#  define CT_DEBUG_BUILD  0
#  define CT_PUBLIC_BUILD 0
#endif

typedef int8_t    s8;
typedef int16_t   s16;
typedef int32_t   s32;
typedef int64_t   s64;

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;

typedef uint8_t    b8;
typedef uint16_t   b16;
typedef uint32_t   b32;
typedef uint64_t   b64;

typedef float     f32;
typedef double    f64;

typedef uintptr_t uptr;
typedef intptr_t  sptr;

typedef union u128
{
    struct { u64 upper, lower; };
    u64 bits64[2];
    u32 bits32[4];
} u128;

typedef u128 guid_t;

#define U8_MAX   UINT8_MAX
#define U16_MAX  UINT16_MAX
#define U32_MAX  UINT32_MAX
#define U64_MAX  UINT64_MAX
#define I8_MAX   INT8_MAX
#define I16_MAX  INT16_MAX
#define I32_MAX  INT32_MAX
#define I64_MAX  INT64_MAX
#define F32_MIN -FLT_MAX
#define F32_MAX  FLT_MAX
#define F64_MIN -DBL_MAX
#define F64_MAX  DBL_MAX

#define var_global  static
#define var_persist static

#define fn            extern "C"
#define fn_internal   static
#define fn_inline     static inline

#define _KB(x) (x * 1024)
#define _MB(x) (_KB(x) * 1024)
#define _GB(x) (_MB(x) * 1024)

#define _64KB  _KB(64)
#define _1MB   _MB(1)
#define _2MB   _MB(2)
#define _4MB   _MB(4)
#define _8MB   _MB(8)
#define _16MB  _MB(16)
#define _32MB  _MB(32)
#define _64MB  _MB(64)
#define _128MB _MB(128)
#define _256MB _MB(256)
#define _1GB   _GB(1)

// A hack to determine if platform is little endian
// TODO(enlynn): Do something that isn't so hacky.
#define CT_LITTLE_ENDIAN 0x41424344UL
#define CT_ENDIAN_ORDER  ('ABCD')
#define CT_IS_LITTLE_ENDIAN (CT_ENDIAN_ORDER==CT_LITTLE_ENDIAN)

#define FORWARD_ALIGN(Base, Alignment) (((u64)(Base) + (u64)(Alignment) - 1) & ~((u64)(Alignment) - 1))
// I doubt this is the most efficient way of doing things, but it is easy to understand and does not run the risk of an underflow
#define BACKWARD_ALIGN(Base, Alignment) (ForwardAlign(((Base) + 1), Alignment) - (Alignment))
#define DIVIDE_ALIGN(val, align) (((val) + (align) - 1) / (align))

#define ARRAY_COUNT(array)    (sizeof(array) / sizeof(array[0]))
#define CLAMP(Val, Min, Max) (((Val) < (Min)) ? (Min) : (((Val) > (Max)) ? (Max) : (Val)))

#define FOR_RANGE(Type, VarName, Count)         for (Type VarName = 0;           VarName < (Count); ++VarName)
#define FOR_RANGE_REVERSE(Type, VarName, Count) for (Type VarName = (Count) - 1; VarName >= 0;      --VarName)

#if CT_PLATFORM_WIN32
#  ifdef CT_EXPORT
#    define fn_export __declspec(dllexport)
#  else
#    define fn_export __declspec(dllimport)
#  endif
#else
#  define fn_export
//#  warning "Exporting function from dll not yet supported on platform"
#endif

#define MAKE_APP_VERSION(major, minor, patch) (u32)((major) << 24 | (minor) << 16 | (patch) << 8)

// source: https://hbfs.wordpress.com/2008/08/05/branchless-equivalents-of-simple-functions/
// short for sign extend :p
// forces the compile to use cbw family of instructions (ideally)
fn_inline int
sex(int x)
{
    union
    {
        s64 w;
        struct { s32 lo, hi; } _p;
    } z;
    z.w = x;
    return z._p.hi;
}

fn_inline s32
fast_sign32(s32 v)
{
    return (((u64)-v >> 31) - ((u64)v >> 31));
}

fn_inline s32
fast_sign64(s64 v)
{
    return (((u64)-v >> 63) - ((u64)v >> 63));
}

fn_inline int
fast_abs(int x)
{
    int result;
    result = (x ^ sex(x)) - sex(x);
    return result;
}

fn_inline int
fast_max(int min, int max)
{
    int result;
    result = min + ((max - min) & ~sex(max - min));
    return result;
}

fn_inline int
fast_min(int min, int max)
{
    int result;
    result = max + ((min - max) & sex(min - max));
    return result;
}

/**
* Round up to the next highest power of 2.
* @source: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
*/
fn_inline u32
next_highest_pow2_u32(u32 v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

/**
* Round up to the next highest power of 2.
* @source: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
*/
fn_inline u64
next_highest_pow2_u64(u64 v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    v++;
    return v;
}

#endif //_TYPES_H_
