#pragma once

#include "Types.h"

// Note: based on boost hash_combine.hpp
// See: https://www.boost.org/doc/libs/1_84_0/boost/intrusive/detail/hash_combine.hpp
//

#if defined(_MSC_VER)
#  include <stdlib.h>
#  define INTRUSIVE_HASH_ROTL32(x, r) _rotl(x,r)
#else
#  define INTRUSIVE_HASH_ROTL32(x, r) (x << r) | (x >> (32 - r))
#endif

template <typename size_t>
inline void hash_combine_size_t(size_t& seed, size_t value)
{
	seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

inline void hash_combine_size_t(u32& h1, u32 k1)
{
	constexpr u32 c1 = 0xcc9e2d51;
	constexpr u32 c2 = 0x1b873593;

	k1 *= c1;
	k1 = INTRUSIVE_HASH_ROTL32(k1, 15);
	k1 *= c2;

	h1 ^= k1;
	h1 = INTRUSIVE_HASH_ROTL32(h1, 13);
	h1 = h1 * 5 + 0xe6546b64;
}

inline void hash_combine_size_t(u64& h, u64 k)
{
    constexpr u64 m = UINT64_C(0xc6a4a7935bd1e995);
    constexpr int r = 47;

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;

    // Completely arbitrary number, to prevent 0's from hashing to 0.
    h += 0xe6546b64;
}