#ifndef _HASH_H_
#define _HASH_H_

#include <std/types.h>

// K *keys   = ...; // this is a stretchy buffer
// V *values = ...; // this is a stretchy buffer
// hashtable value_lookup;
//
// array_push_back(values, new_value);
// hastable_insert(hash_key(key), arrlen_len(values) - 1);
//

typedef struct
{
    int  count;
    int  capacity;
    u64 *key_table;
    u32 *index_table;
} hashtable_t;

fn_export u32  hash_u32(const void *key, const int key_len);
fn_export u64  hash_u64(const void *key, const int key_len);
fn_export u128 hash_u128(const void *key, const int key_len);

fn_export size_t hash_combine_size_t(const size_t seed, const size_t value);
fn_export u32    hash_combine_u32(const u32 seed, const u32 value);
fn_export u64    hash_combine_u64(const u64 seed, const u64 value);

fn_export void hashtable_init(int initial_capacity, hashtable_t *hashtable);
fn_export void hashtable_deinit(hashtable_t *hashtable);
fn_export bool hashtable_insert(hashtable_t *hashtable, u64 key, u32 value_index);
fn_export bool hashtable_get(hashtable_t *hashtable, u64 key, u32 *value_index);
// todo: hashtable_delete

#endif //_HASH_H_
