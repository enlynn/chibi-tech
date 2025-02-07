#include "array.h"

#include <std/mem.h>

#include <assert.h>

struct array_header_t
{
    int count;
    int stride;
    int capacity;
};

#define HEADER_SIZE sizeof(struct array_header_t)

fn_inline struct array_header_t*
ptr_to_header(void *ptr)
{
    return (struct array_header_t*)((u8*)ptr - HEADER_SIZE);
}

fn_inline void*
header_to_ptr(struct array_header_t* header)
{
    return (void*)((u8*)header + HEADER_SIZE);
}

fn_export int
ct_array_get_len(void *ptr)
{
    struct array_header_t* header = ptr_to_header(ptr);
    return header->count;
}

fn_export int
ct_array_get_cap(void *ptr)
{
    struct array_header_t* header = ptr_to_header(ptr);
    return header->capacity;
}

fn_inline int
ct_array_get_stride(void *ptr)
{
    struct array_header_t* header = ptr_to_header(ptr);
    return header->stride;
}

fn_export void
ct_array_init(void **ptr, int stride, int capacity)
{
    if (capacity <= 0) capacity = c_array_default_capacity;

    int alloc_size = HEADER_SIZE + (stride * capacity);
    struct array_header_t *header = (struct array_header_t*)SYS_ALLOC(alloc_size);
    header->count    = 0;
    header->stride   = stride;
    header->capacity = capacity;

    *ptr = header_to_ptr(header);
}

fn_export void
ct_array_free(void **ptr)
{
    struct array_header_t *header = ptr_to_header(*ptr);
    SYS_FREE(header);
}

fn_internal void
ct_array_grow(void **ptr, int new_cap)
{
    const int cap = ct_array_get_cap(*ptr);
    if (new_cap <= cap)
    {
        return;
    }

    void *current_base = *ptr;
    const int len     = ct_array_get_len(current_base);
    const int stride  = ct_array_get_stride(current_base);

    void *new_array = NULL;
    ct_array_init(&new_array, stride, new_cap);

    struct array_header_t* new_header = ptr_to_header(new_array);
    new_header->count = len;

    mem_copy(new_array, current_base, stride * len);
}

fn_export void
ct_array_insert(void **ptr, int index, void *value)
{
    const int len = ct_array_get_len(*ptr);
    const int cap = ct_array_get_cap(*ptr);
    assert(index <= len);

    if (len + 1 > cap) ct_array_grow(ptr, cap * 2);

    struct array_header_t* header = ptr_to_header(*ptr);
    header->count = len + 1;

    u8 *iter = (u8*)*ptr;
    for (int ins_idx = len; ins_idx > index; --ins_idx)
    {
        u8 *src = iter + ((ins_idx - 1) * header->stride);
        u8 *dst = iter + ((ins_idx - 0) * header->stride);
        mem_copy(dst, src, header->stride);
    }

    mem_copy(iter + (index * header->stride), value, header->stride);
}

fn_export void
ct_array_ordered_remove(void *ptr, int index)
{
    struct array_header_t* header = ptr_to_header(ptr);

    if (header->count == 0 || index >= header->count)
    {
        return;
    }

    u8 *iter = (u8*)ptr;
    for (int del_idx = header->count - 1; del_idx >= index; ++del_idx)
    {
        u8 *dst = iter + ((del_idx - 1) * header->stride);
        u8 *src = iter + ((del_idx - 0) * header->stride);
        mem_copy(dst, src, header->stride);
    }

    header->count -= 1;
}

fn_export void
ct_array_unordered_remove(void *ptr, int index)
{
    struct array_header_t* header = ptr_to_header(ptr);

    if (header->count == 0 || index >= header->count)
    {
        return;
    }

    if (header->count > 1)
    {
        // swap the back value with the value we want to remove.
        u8 *dst = (u8*)ptr + (index * header->stride);
        u8 *src = (u8*)ptr + ((header->count - 1) * header->stride);
        mem_copy(dst, src, header->stride);
    }

    header->count -= 1;
}

fn_export void
ct_array_set_len(void **ptr, int new_len)
{
    ct_array_grow(ptr, new_len);

    struct array_header_t *header = ptr_to_header(*ptr);
    header->count = new_len;
}

fn_export void
ct_array_set_capacity(void **ptr, int new_cap)
{
    ct_array_grow(ptr, new_cap);

    struct array_header_t *header = ptr_to_header(*ptr);
    header->capacity = new_cap;
    header->count    = fast_min(header->count, header->capacity);
}
