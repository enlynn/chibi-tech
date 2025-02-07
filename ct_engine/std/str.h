#ifndef _STR_H_
#define _STR_H_

#include "types.h"

typedef struct {
	union
    {
        struct
        {
            char Ptr[16];
        } stack;

        struct
        {
            char* ptr;
            //
            // If stack allocation:
            // - Field is overwritten and unused
            //
            // If heap allocation:
            // - Field is treated as the length
            //
            u64   length;
        } heap;
    } data;

    //
    // If stack allocation:
    // - Up to first 7 bytes is extra storage for sptr
    // - Length is stored as 2 * (MaxShortLen - Len)
    // ---- Bottom bit will always be zero in this case
    // ---- At max length (23 bytes), length acts as the NULL byte
    //
    // If heap allocation:
    // - This field is treated as the heap size for the allocation
    // - Assume all allocations are 8 byte aligned, therefore bottom
    //   bottom three bits can be used as flag bits
    // -- Bit 0 unused
    // -- Bit 1 unused
    // -- Bit 2 heap flag
    //
    // ;tldr
    //
    // - Stack: encoded length
    // - Heap:  capacity
    //
    union
    {
        struct
        {
            u64 encoded_len;
        } stack;

        struct
        {
            u64 capacity;
        } heap;
    } footer;
} str8_t;

fn_export int utf8_to_utf16(const char* utf8, int utf8_len, wchar_t* utf16, int utf16_buff_len);
fn_export int utf16_to_utf8(const wchar_t* utf16, int utf16_len, char* utf8, int utf8_buf_len);

fn_export int str_len(const char* str);
fn_export int str16_len(const wchar_t* str);
fn_export bool str_cmp(const char* str_a, const char* str_b);

#endif //_STR_H_
