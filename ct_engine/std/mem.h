#ifndef _STDMEM_H_
#define _STDMEM_H_

#include "types.h"

#if CT_PLATFORM_WIN32
#  include <stdalign.h>
#endif

//
// STD Library
//

#define SYS_ALLOC_STRUCT(type)     SYS_REALLOC(NULL, sizeof(type))
#define SYS_ALLOC_ARRAY(type, num) SYS_REALLOC(NULL, sizeof(type) * (num))
#define SYS_ALLOC(size)            SYS_REALLOC(NULL, size)

#define SYS_REALLOC(ptr, size) sys_realloc(ptr, size)
#define SYS_FREE(ptr)          do { sys_free(ptr); (ptr) = NULL; } while (0);

fn_export void *sys_realloc(void *old_ptr, size_t new_size);
fn_export void  sys_free(void *ptr);

#define ZERO_STRUCT(ptr)      memzero(ptr, sizeof(typeof(*(ptr))))
#define ZERO_ARRAY(ptr,count) memzero(ptr, (count) * sizeof(typeof((ptr)[0])))

fn_export void memzero(void *ptr, size_t num_bytes);

fn_export void mem_copy(void *dst, const void *src, size_t num_bytes);

fn_export bool is_pointer_within_range(void *base_ptr, void *end_ptr, void *ptr);

enum virtual_alloc_flag {
    VIRTUAL_ALLOC_NONE    = 0x00, //will allocate with VIRTAL_ALLOC_DEFAULT
    VIRTUAL_ALLOC_RESERVE = 0x01,
    VIRTUAL_ALLOC_COMMIT  = 0x02,
    VIRTUAL_ALLOC_DEFAULT = VIRTUAL_ALLOC_RESERVE | VIRTUAL_ALLOC_COMMIT,
};

fn_export void* virtual_alloc(void *base_ptr, size_t size, enum virtual_alloc_flag flags);
fn_export void  virtual_free(void *ptr, size_t size /* required for mmap */);

//
// Allocator interface
//

struct allocator_t;
typedef void  *(*allocator_alloc_f)(struct allocator_t *self, size_t size, size_t alignment);
typedef void   (*allocator_free_f)(struct allocator_t *self, void *ptr);
typedef size_t (*allocator_get_allocation_size_f)(struct allocator_t *self, void *ptr); //note: not guarenteed to work with all allocators

typedef struct allocator_t
{
    allocator_alloc_f               alloc;
    allocator_free_f                free;
    allocator_get_allocation_size_f get_allocation_size;
} allocator_t;

#define CT_ALLOC_STRUCT(allocator, type)       (type*)allocator_alloc(allocator,           sizeof(type), alignof(type))
#define CT_ALLOC_ARRAY(allocator, type, count) (type*)allocator_alloc(allocator, (count) * sizeof(type), alignof(type))
#define CT_ALLOC_RAW(allocator, bytes)                allocator_alloc(allocator,                  bytes, alignof(void*))

#define CT_FREE(allocator, ptr)                allocator_free(allocator, (void*)ptr)
#define CT_GET_ALLOCATION_SIZE(allocator, ptr) allocator_get_allocation_size(allocator, (void*)ptr)

fn_inline void*
allocator_alloc(allocator_t *self, size_t size, size_t alignment)
{
    return self->alloc(self, size, alignment);
}

fn_inline void
allocator_free(allocator_t *self, void *ptr)
{
    self->free(self, ptr);
}

fn_inline size_t
allocator_get_allocation_size(allocator_t *self, void *ptr)
{
    return self->get_allocation_size(self, ptr);
}

//
// Allocators
//

fn_export allocator_t* create_pool_allocator(int num_objects, u32 stride);

//
// Memory Subsystem
//

void create_memory_subsystem();
void destroy_memory_subsystem();

void reset_frame_allocator(u64 frame_id);
fn_export allocator_t* get_frame_allocator();

#endif //_STDMEM_H_
