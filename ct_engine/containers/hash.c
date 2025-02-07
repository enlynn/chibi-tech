#include "hash.h"
#include <std/mem.h>


// Note: based on boost hash_combine.hpp
// See: https://www.boost.org/doc/libs/1_84_0/boost/intrusive/detail/hash_combine.hpp
//

#if defined(_MSC_VER)
#  include <stdlib.h>
#  define INTRUSIVE_HASH_ROTL32(x, r) _rotl(x,r)
#else
#  define INTRUSIVE_HASH_ROTL32(x, r) (x << r) | (x >> (32 - r))
#endif

fn_export size_t
hash_combine_size_t(const size_t seed, const size_t value)
{
    return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

fn_export u32
hash_combine_u32(const u32 seed, const u32 value)
{
    const u32 c1 = 0xcc9e2d51;
	const u32 c2 = 0x1b873593;

	u32 right = value;
	right *= c1;
	right = INTRUSIVE_HASH_ROTL32(right, 15);
	right *= c2;

	u32 result = seed;
	result ^= right;
	result = INTRUSIVE_HASH_ROTL32(result, 13);
	result = result * 5 + 0xe6546b64;

	return result;
}

fn_export u64
hash_combine_u64(const u64 seed, const u64 value)
{
    const u64 m = UINT64_C(0xc6a4a7935bd1e995);
    const int r = 47;

    u64 right = value;
    right *= m;
    right ^= right >> r;
    right *= m;

    u64 result = seed;
    result ^= right;
    result *= m;

    // Completely arbitrary number, to prevent 0's from hashing to 0.
    result += 0xe6546b64;
    return result;
}

#if defined(_MSC_VER) && (_MSC_VER < 1600)

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;

// Other compilers

#else	// defined(_MSC_VER)

#include <stdint.h>

#endif // !defined(_MSC_VER)

//-----------------------------------------------------------------------------
// Murmmur Hash

fn_internal void MurmurHash3_x64_128 ( const void * key, int len, uint32_t seed, void * out );

fn_inline u32
murmmurhash_to_u32(u128 hash)
{
    u32 result = hash_combine_u32(hash.bits32[0], hash.bits32[1]);
    result = hash_combine_u32(result, hash.bits32[2]);
    result = hash_combine_u32(result, hash.bits32[3]);
    return result;
}

fn_inline u64
murmmurhash_to_u64(u128 hash)
{
    return hash_combine_u64(hash.bits64[0], hash.bits64[1]);
}

//-----------------------------------------------------------------------------
// Platform-specific functions and macros

// Microsoft Visual Studio

#if defined(_MSC_VER)

#include <stdlib.h>

#define ROTL32(x,y)	_rotl(x,y)
#define ROTL64(x,y)	_rotl64(x,y)

#define BIG_CONSTANT(x) (x)

// Other compilers???

#else	// defined(_MSC_VER)

fn_inline uint32_t rotl32 ( uint32_t x, int8_t r )
{
  return (x << r) | (x >> (32 - r));
}

fn_inline uint64_t rotl64 ( uint64_t x, int8_t r )
{
  return (x << r) | (x >> (64 - r));
}

#define	ROTL32(x,y)	rotl32(x,y)
#define ROTL64(x,y)	rotl64(x,y)

#define BIG_CONSTANT(x) (x##LLU)

#endif // !defined(_MSC_VER)

// Block read - if your platform needs to do endian-swapping or can only
// handle aligned reads, do the conversion here
fn_inline uint32_t
getblock32 ( const uint32_t * p, int i )
{
  return p[i];
}

fn_inline uint64_t
getblock64 ( const uint64_t * p, int i )
{
  return p[i];
}

// Finalization mix - force all bits of a hash block to avalanche
fn_inline uint32_t
fmix32 ( uint32_t h )
{
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;

  return h;
}

//----------

fn_inline uint64_t
fmix64 ( uint64_t k )
{
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xff51afd7ed558ccd);
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
  k ^= k >> 33;

  return k;
}

fn_internal void
MurmurHash3_x64_128(const void * key, const int len, const uint32_t seed, void * out)
{
  const uint8_t * data = (const uint8_t*)key;
  const int nblocks = len / 16;

  uint64_t h1 = seed;
  uint64_t h2 = seed;

  const uint64_t c1 = BIG_CONSTANT(0x87c37b91114253d5);
  const uint64_t c2 = BIG_CONSTANT(0x4cf5ad432745937f);

  //----------
  // body

  const uint64_t * blocks = (const uint64_t *)(data);

  for(int i = 0; i < nblocks; i++)
  {
    uint64_t k1 = getblock64(blocks,i*2+0);
    uint64_t k2 = getblock64(blocks,i*2+1);

    k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;

    h1 = ROTL64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;

    k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;

    h2 = ROTL64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;
  }

  //----------
  // tail

  const uint8_t * tail = (const uint8_t*)(data + nblocks*16);

  uint64_t k1 = 0;
  uint64_t k2 = 0;

  switch(len & 15)
  {
  case 15: k2 ^= ((uint64_t)tail[14]) << 48;
  case 14: k2 ^= ((uint64_t)tail[13]) << 40;
  case 13: k2 ^= ((uint64_t)tail[12]) << 32;
  case 12: k2 ^= ((uint64_t)tail[11]) << 24;
  case 11: k2 ^= ((uint64_t)tail[10]) << 16;
  case 10: k2 ^= ((uint64_t)tail[ 9]) << 8;
  case  9: k2 ^= ((uint64_t)tail[ 8]) << 0;
           k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;

  case  8: k1 ^= ((uint64_t)tail[ 7]) << 56;
  case  7: k1 ^= ((uint64_t)tail[ 6]) << 48;
  case  6: k1 ^= ((uint64_t)tail[ 5]) << 40;
  case  5: k1 ^= ((uint64_t)tail[ 4]) << 32;
  case  4: k1 ^= ((uint64_t)tail[ 3]) << 24;
  case  3: k1 ^= ((uint64_t)tail[ 2]) << 16;
  case  2: k1 ^= ((uint64_t)tail[ 1]) << 8;
  case  1: k1 ^= ((uint64_t)tail[ 0]) << 0;
           k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;
  };

  //----------
  // finalization

  h1 ^= len; h2 ^= len;

  h1 += h2;
  h2 += h1;

  h1 = fmix64(h1);
  h2 = fmix64(h2);

  h1 += h2;
  h2 += h1;

  ((uint64_t*)out)[0] = h1;
  ((uint64_t*)out)[1] = h2;
}

//-----------------------------------------------------------------------------
// Hash Implementation

//todo(enlynn): i should come up with a better seed i suppose
const u32 c_hash_seed = 5724896;

//MurmurHash3_x64_128(const void * key, const int len, const uint32_t seed, void * out)

fn_export u32
hash_u32(const void *key, const int key_len)
{
    u128 result;
    MurmurHash3_x64_128(key, key_len, c_hash_seed, &result);

    return murmmurhash_to_u32(result);
}

fn_export u64
hash_u64(const void *key, const int key_len)
{
    u128 result;
    MurmurHash3_x64_128(key, key_len, c_hash_seed, &result);

    return murmmurhash_to_u64(result);
}

fn_export u128
hash_u128(const void *key, const int key_len)
{
    u128 result;
    MurmurHash3_x64_128(key, key_len, c_hash_seed, &result);

    return result;
}

//-----------------------------------------------------------------------------
// Hashtable Implementation

static const f32 c_hashtable_load      = 0.70f;
static const u64 c_hashtable_empty     = U64_MAX;
static const u64 c_hashtable_tombstone = U64_MAX - 1;

fn_export void
hashtable_init(int initial_capacity, hashtable_t *hashtable)
{
    const int final_size = initial_capacity * (sizeof(u64) + sizeof(u32));

    u8 *base_ptr = (u8*)SYS_ALLOC(final_size);
    hashtable->key_table   = (u64*)base_ptr;
    hashtable->index_table = (u32*)(hashtable->key_table + initial_capacity);
    hashtable->count       = 0;
    hashtable->capacity    = initial_capacity;
}

fn_export void
hashtable_deinit(hashtable_t *hashtable)
{
    SYS_FREE(hashtable->key_table);
    memzero(hashtable, sizeof(hashtable_t));
}

fn_internal void
hashtable_grow(hashtable_t *hashtable, int new_capacity)
{
    if (new_capacity <= hashtable->capacity) return;

    hashtable_t temp_table;
    hashtable_init(new_capacity, &temp_table);

    for (int index = 0; index < hashtable->capacity; ++index)
    {
        if (hashtable->key_table[index] == c_hashtable_empty || hashtable->key_table[index] == c_hashtable_tombstone)
        {
            continue;
        }

        hashtable_insert(&temp_table, hashtable->key_table[index], hashtable->index_table[index]);
    }

    void *base_ptr = hashtable->key_table;

    hashtable->key_table   = temp_table.key_table;
    hashtable->index_table = temp_table.index_table;
    hashtable->capacity    = new_capacity;

    SYS_FREE(base_ptr);
}

fn_export bool
hashtable_insert(hashtable_t *hashtable, u64 key, u32 value_index)
{
    bool result = false;

    f32 load_factor = (f32)hashtable->count / (f32)hashtable->capacity;
    if (load_factor > c_hashtable_load)
    {
        hashtable_grow(hashtable, hashtable->capacity / (c_hashtable_load * 0.5f));
    }

    u64 initial_index = key % hashtable->capacity;
    u64 iter_index    = initial_index;

    do
    {
        if (hashtable->key_table[iter_index] == c_hashtable_empty || hashtable->key_table[iter_index] == c_hashtable_tombstone)
        {
            hashtable->key_table[iter_index]   = key;
            hashtable->index_table[iter_index] = value_index;
            result = true;
            break;
        }

        iter_index = (iter_index + 1) % hashtable->capacity;
    } while (iter_index != initial_index);

    return result;
}

fn_export bool
hashtable_get(hashtable_t *hashtable, u64 key, u32 *value_index)
{
    bool result = false;

    u64 initial_index = key % hashtable->capacity;
    u64 iter_index    = initial_index;

    do
    {
        if (hashtable->key_table[iter_index] == c_hashtable_empty)
        {
            break;
        }
        else if (hashtable->key_table[iter_index] == key)
        {
            *value_index = hashtable->index_table[iter_index];
            result = true;
            break;
        }

        iter_index = (iter_index + 1) % hashtable->capacity;
    } while (iter_index != initial_index);

    return result;
}
