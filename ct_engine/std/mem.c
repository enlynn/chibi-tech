#include "mem.h"
#include "cassert.h"

#include <stdlib.h>

fn_export void*
sys_realloc(void *old_ptr, size_t new_size) {
	return realloc(old_ptr, new_size);
}

fn_export void
sys_free(void *ptr) {
	free(ptr);
}

fn_export void
memzero(void *ptr, size_t num_bytes)
{
	const u8 *end_ptr = (u8*)ptr + num_bytes;
	for (u8 *iter = (u8*)ptr; iter < end_ptr; ++iter) *iter = 0;
}

fn_export void
mem_copy(void *dst, const void *src, size_t num_bytes)
{
    const u8 *end_ptr = (u8*)src + num_bytes;
	for (u8 *src_iter = (u8*)src, *dst_iter = (u8*)dst; src_iter < end_ptr; ++src_iter, ++dst_iter)
	{
	   *dst_iter = *src_iter;
	}
}

//
// Allocator helpers
//

#define ALLOCATOR_ALLOC_F(func)          void*  func(struct allocator_t *self, size_t size, size_t alignment)
#define ALLOCATOR_FREE_F(func)           void   func(struct allocator_t *self, void *ptr)
#define ALLOCATOR_RESET_F(func)          void   func(struct allocator_t *self)
#define ALLOCATOR_GET_ALLOC_SIZE_F(func) size_t func(struct allocator_t *self, void *ptr)

fn_export bool
is_pointer_within_range(void *base_ptr, void *end_ptr, void *ptr)
{
    return (uintptr_t)ptr >= (uintptr_t)base_ptr && (uintptr_t)ptr < (uintptr_t)end_ptr;
}

ALLOCATOR_ALLOC_F(allocator_alloc_stub)
{
	return NULL;
}

ALLOCATOR_FREE_F(allocator_free_stub)
{
	return;
}

ALLOCATOR_RESET_F(allocator_reset_stub)
{
	return;
}

ALLOCATOR_GET_ALLOC_SIZE_F(allocator_get_alloc_stub)
{
	return 0;
}

fn_internal void
make_empty_allocator(struct allocator_t *allocator)
{
	allocator->alloc               = allocator_alloc_stub;
	allocator->free                = allocator_free_stub;
	allocator->get_allocation_size = allocator_get_alloc_stub;
}

//
// Free List Allocator
//

typedef struct heap_header_t {
	u32                   size;
	struct heap_header_t *next;
	struct heap_header_t *prev;
} heap_header_t;

typedef struct {
	u8            *base_ptr;
	u8            *end_ptr;
	heap_header_t *free_list;
	//todo: should free_memory refer to the total memory within the free list
	//      or the memory that excludes the "size" field of thhe header?
	u32            free_memory;
	u32            num_allocations;
} heap_allocator_t;

fn_internal bool
heap_try_merge_header(heap_header_t *base_header, heap_header_t *header_to_merge)
{
	ASSERT(base_header != NULL);

	if (header_to_merge == NULL)
	{
		return false;
	}

	if (((u8*)base_header + base_header->size) != (u8*)header_to_merge)
	{ // these are not adjacent blocks of memory
		return false;
	}

	base_header->size += header_to_merge->size;
	base_header->next  = header_to_merge->next;

	if (base_header->next != NULL)
	{
		base_header->next->prev = base_header;
	}

	header_to_merge->size = 0;
	header_to_merge->next = NULL;
	header_to_merge->prev = NULL;
	return true;
}

fn_internal void
heap_insert_free_list(heap_allocator_t *self, void *base_ptr, u32 size)
{
	heap_header_t *header = (heap_header_t*)base_ptr;
	header->size = size;
	header->prev = NULL;
	header->next = NULL;

	if (!self->free_list)
	{
		self->free_list        = header;
		self->free_memory     += header->size;
		self->num_allocations -= 1;
		return;
	}

	heap_header_t *iter = self->free_list;
	heap_header_t *last = NULL;
	
	while (iter != NULL && header > iter) 
	{
		last = iter;
		iter = iter->next;
	}

	if (!iter)
	{ //end of the free list
		last->next   = header;
		header->prev = last;

		heap_try_merge_header(last, header);
	}
	else if (iter == self->free_list)
	{ // insert to the front of the free list
		ASSERT(iter->prev == NULL);
		iter->prev      = header;
		header->next    = iter;
		self->free_list = header;

		heap_try_merge_header(self->free_list, self->free_list->next);
	}
	else
	{
		ASSERT(last->next == iter->prev);

		last->next   = header;
		header->prev = last;

		header->next = iter;
		iter->prev   = header;

		if (heap_try_merge_header(last, header))
		{
			heap_try_merge_header(last, last->next);
		}
		else
		{
			heap_try_merge_header(header, iter);
		}
	}

	self->free_memory     += header->size;
	self->num_allocations -= 1;
}

fn_internal void
heap_init(heap_allocator_t *self, void *base_ptr, u32 size)
{
	ASSERT(size >= sizeof(heap_header_t));

	self->base_ptr        = (u8*)base_ptr;
	self->end_ptr         = self->base_ptr + size;
	self->free_memory     = 0;
	self->num_allocations = 0;
	self->free_list       = NULL;

	heap_insert_free_list(self, self->base_ptr, size);
}

ALLOCATOR_ALLOC_F(allocator_heap_alloc)
{
	return NULL;
}

ALLOCATOR_FREE_F(allocator_heap_free)
{
	return;
}

ALLOCATOR_RESET_F(allocator_heap_reset)
{
	return;
}

ALLOCATOR_GET_ALLOC_SIZE_F(allocator_heap_get_alloc)
{
	return 0;
}


//
// Arena Allocator
//

struct memory_arena_t {
	u8 *base;
	u8 *offset;
	u8 *end;
};

fn_internal void
arena_init(struct memory_arena_t *arena, void *base, size_t size) {
	arena->base   = base;
	arena->offset = arena->base;
	arena->end    = arena->base + size;
}

fn_internal void*
arena_alloc(struct memory_arena_t *arena, size_t size) {
	void *result = NULL;
	if (arena->offset + size < arena->end) {
		result = arena->offset;
		arena->offset += size;
	}
	return result;
}

fn_internal void
arena_reset(struct memory_arena_t *arena) {
	arena->offset = arena->base;
}

fn_internal void*
allocator_arena_alloc(struct allocator_t *self, size_t size, size_t alignment)
{
    struct memory_arena_t *arena = (struct memory_arena_t*)(self + 1);
    return arena_alloc(arena, size);
}

fn_internal void
allocator_arena_free(struct allocator_t *self, void *ptr)
{
    struct memory_arena_t *arena = (struct memory_arena_t*)(self + 1);
    arena_reset(arena);
}

fn_internal size_t
allocator_arena_get_allocation_size(struct allocator_t *self, void *ptr)
{
    //struct memory_arena_t *arena = (struct memory_arena_t*)(self + 1);
    return 0;
}

//
// Pool Allocator
//

struct memory_pool_t {
	u32    stride;
	u8    *base;
	u8    *end;
	void **free_list;
};

fn_internal void
pool_init(struct memory_pool_t *pool, void *base, int num_objects, u32 stride) {
	const int size = num_objects * stride;
	
	ASSERT(size   >= sizeof(void*));
	ASSERT(stride >= sizeof(void*));

	pool->stride    = stride;
	pool->base      = base;
	pool->end       = pool->base + size;
	pool->free_list = (void**)base;

	// initialize the free list.
    void **iter = pool->free_list;
    for (int i = 0; i < num_objects - 1; ++i) {
        *iter = (u8*)iter + pool->stride;
        iter  = (void**)(*iter);
    }
    *iter = NULL;
}

ALLOCATOR_ALLOC_F(allocator_pool_alloc)
{
	struct memory_pool_t *pool = (struct memory_pool_t*)(self + 1);
	
	if (pool->free_list == NULL) {
		return NULL;
	}

	void *result = pool->free_list;
    pool->free_list = (void**)(*pool->free_list);

    return result;
}

ALLOCATOR_FREE_F(allocator_pool_free)
{
	struct memory_pool_t *pool = (struct memory_pool_t*)(self + 1);
	
	ASSERT(is_pointer_within_range(pool->base, pool->end, ptr));

	*((void**)ptr)  = pool->free_list;
    pool->free_list = (void**)ptr;

	return;
}

ALLOCATOR_RESET_F(allocator_pool_reset)
{
	return;
}

ALLOCATOR_GET_ALLOC_SIZE_F(allocator_pool_get_alloc)
{
	return 0;
}

//
// Memory Subsystem
//

var_global u8          *g_base_memory_ptr  = NULL;
var_global size_t       g_base_memory_size = 0;
var_global allocator_t *g_frame_allocator  = NULL;

void
create_memory_subsystem()
{
    // for now, only allocator memory for the frame allocator
    g_base_memory_size = sizeof(allocator_t) + sizeof(struct memory_arena_t) + _2MB;
    g_base_memory_ptr  = (u8*)virtual_alloc(NULL, g_base_memory_size, VIRTUAL_ALLOC_DEFAULT);

    struct memory_arena_t *frame_arena = (struct memory_arena_t*)(g_base_memory_ptr + sizeof(allocator_t));
    arena_init(frame_arena, (void*)(frame_arena + 1), _2MB);

    g_frame_allocator = (allocator_t*)g_base_memory_ptr;
    g_frame_allocator->alloc               = allocator_arena_alloc;
    g_frame_allocator->free                = allocator_arena_free;
    g_frame_allocator->get_allocation_size = allocator_arena_get_allocation_size;
}

void
destroy_memory_subsystem()
{
    virtual_free(g_base_memory_ptr, g_base_memory_size);

    g_base_memory_ptr  = NULL;
    g_base_memory_size = 0;
    g_frame_allocator  = NULL;
}

void
reset_frame_allocator(u64 frame_id)
{
    // for now, the frame_id does not mean anything
    allocator_free(g_frame_allocator, NULL);
}

fn_export allocator_t*
get_frame_allocator()
{
    //todo: will want to eventually take into account the current thread
    return g_frame_allocator;
}

fn_export allocator_t*
create_pool_allocator(int num_objects, u32 stride)
{
	u64 memsize  = sizeof(allocator_t) + sizeof(struct memory_pool_t) + (stride * num_objects);
	u8* base_ptr = (u8*)SYS_ALLOC(memsize); //todo: allocate from global memory

	struct memory_pool_t *pool = (struct memory_pool_t*)(base_ptr + sizeof(allocator_t));
	pool_init(pool, (void*)(pool + 1), num_objects, stride);

	allocator_t *result = (allocator_t*)base_ptr;
	result->alloc               = allocator_pool_alloc;
	result->free                = allocator_pool_free;
	result->get_allocation_size = allocator_pool_get_alloc;
	return result;
}