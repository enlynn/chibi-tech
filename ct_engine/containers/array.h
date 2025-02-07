#ifndef _ARRAY_H_
#define _ARRAY_H_

#include <std/types.h>

// Dynamic Array type modelled after Sean Barret's implementation.

static const int c_array_default_capacity = 10;

#define array_init(ptr)                     array_init_with_capacity(ptr, c_array_default_capacity)
#define array_init_with_capacity(ptr, cap)  ct_array_init(&ptr, sizeof(typeof(*ptr)), cap)
#define array_free(ptr)                     do { ct_array_free(&ptr); ptr = NULL; } while (0);

#define array_set_len(ptr, len)             ct_array_set_len(&ptr, len)
#define array_set_capacity(ptr, cap)        ct_array_set_len(&ptr, cap)

#define array_get_len(ptr)                  ct_array_get_len(ptr)
#define array_get_capacity(ptr)             ct_array_get_capacity(ptr)

#define array_push_front(ptr, val)          array_insert(ptr,                  0, val)
#define array_push_back(ptr, val)           array_insert(ptr, array_get_len(ptr), val)
#define array_insert(ptr, index, val)       ct_array_insert(&ptr, index, val)

#define array_pop_front(ptr)                array_ordered_remove(ptr, 0)
#define array_pop_back(ptr)                 array_ordered_remove(ptr, array_get_len(ptr) - 1)
#define array_ordered_remove(ptr, index)    ct_array_ordered_remove(ptr, index)
#define array_unordered_remove(ptr, index)  ct_array_unordered_remove(ptr, index)

fn_export void ct_array_init(void **ptr, int stride, int capacity);
fn_export void ct_array_free(void **ptr);

fn_export void ct_array_insert(void **ptr, int index, void *value);
fn_export void ct_array_ordered_remove(void *ptr, int index);
fn_export void ct_array_unordered_remove(void *ptr, int index);

fn_export void ct_array_set_len(void **ptr, int new_len);
fn_export void ct_array_set_capacity(void **ptr, int new_cap);

fn_export int ct_array_get_len(void *ptr);
fn_export int ct_array_get_capacity(void *ptr);

#endif //_ARRAY_H_
