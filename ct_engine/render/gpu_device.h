#ifndef _GPU_DEVICE_H_
#define _GPU_DEVICE_H_

#include <std/types.h>

struct os_window_surface;

struct gpu_device_t;
struct gpu_descriptor_pool_t;
struct gpu_descriptor_write_t;
struct gpu_paged_descriptor_pool_t;
struct gpu_descriptor_layout_t;
struct gpu_descriptor_set_t;
struct gpu_image_t;
struct gpu_buffer_t;

//
// Gpu Descriptors
//

#define MAX_DESCRIPTOR_BINDINGS 32

typedef enum {
	GPU_SHADER_STAGE_VERTEX       = 0x01,
	GPU_SHADER_STAGE_FRAGMENT     = 0x02,
	GPU_SHADER_STAGE_COMPUTE      = 0x04,
	GPU_SHADER_STAGE_ALL_GRAPHICS = GPU_SHADER_STAGE_VERTEX|GPU_SHADER_STAGE_FRAGMENT,
	GPU_SHADER_STAGE_ALL          = GPU_SHADER_STAGE_ALL_GRAPHICS|GPU_SHADER_STAGE_COMPUTE,
} gpu_shader_stage_flags_t;

typedef enum {
	DESCRIPTOR_TYPE_SAMPLER,
	DESCRIPTOR_TYPE_COMBINED_SAMPLER,

	DESCRIPTOR_TYPE_SAMPLED_IMAGE,
	DESCRIPTOR_TYPE_STORAGE_IMAGE,

	DESCRIPTOR_TYPE_UNIFORM_BUFFER, // dynamic variant
	DESCRIPTOR_TYPE_STORAGE_BUFFER, // dynamic variant

	DESCRIPTOR_TYPE_COUNT,
} gpu_descriptor_type_t;

typedef enum {
	DESCRIPTOR_LAYOUT_FLAG_UPDATE_AFTER_BIND = 0x01,
	DESCRIPTOR_LAYOUT_FLAG_PUSH_DESCRIPTOR   = 0x02,
	DESCRIPTOR_LAYOUT_FLAG_DESCRIPTOR_BUFFER = 0x04,
} gpu_descriptor_layout_flags_t;

typedef enum {
	DESCRIPTOR_POOL_FLAG_NONE       = 0x00,
	DESCRIPTOR_POOL_FLAG_ALLOW_FREE = 0x01,
} gpu_descriptor_pool_flags_t;

typedef struct {
	gpu_descriptor_type_t type;
	f32                   ratio;
} gpu_descriptor_pool_size_ratio_t;

typedef struct {
	u32                   binding;
	gpu_descriptor_type_t type;
} gpu_descriptor_binding_t;

typedef struct {
	gpu_descriptor_type_t descriptor_type;
	u32                   binding;
	struct gpu_image_t   *image;                     // for image descriptor writes, leave NULL for buffer writes
	struct gpu_buffer_t  *buffer;                    // for buffer descriptor writes, leave NULL for image writes
	u64                   size_of_buffer_to_update;  // for buffer descriptor writes, leave NULL for image writes
	u64                   buffer_offset;             // for buffer descriptor writes, leave NULL for image writes
} gpu_descriptor_write_t;

fn_inline gpu_descriptor_write_t 
gpu_descriptor_write_image(
	struct gpu_descriptor_write_t *writer,
	gpu_descriptor_type_t          descriptor_type,
	struct gpu_image_t            *image,           //image_view, image_sampler, image_layout
	u32                            binding)
{
	gpu_descriptor_write_t result = {};
	result.descriptor_type          = descriptor_type;
	result.binding                  = binding;
	result.image                    = image;
	result.buffer                   = NULL;
	result.size_of_buffer_to_update = 0;
	result.buffer_offset            = 0;
	return result;
}   

fn_inline gpu_descriptor_write_t 
gpu_descriptor_write_buffer(
	struct gpu_descriptor_write_t *writer,
	gpu_descriptor_type_t          descriptor_type,
	struct gpu_buffer_t           *buffer,                   //VkBuffer
	u64                            size_of_buffer_to_update,
	u64                            buffer_offset,
	u32                            binding)
{
	gpu_descriptor_write_t result = {};
	result.descriptor_type          = descriptor_type;
	result.binding                  = binding;
	result.image                    = NULL;
	result.buffer                   = buffer;
	result.size_of_buffer_to_update = size_of_buffer_to_update;
	result.buffer_offset            = buffer_offset;
	return result;
}

fn_inline gpu_descriptor_write_t 
gpu_descriptor_write_combined_image_sampler(struct gpu_descriptor_write_t *writer, struct gpu_image_t *image, u32 binding)
{
	return gpu_descriptor_write_image(writer, DESCRIPTOR_TYPE_COMBINED_SAMPLER, image, binding);
}

fn_inline gpu_descriptor_write_t
gpu_descriptor_write_storage_image(struct gpu_descriptor_write_t *writer, struct gpu_image_t *image, u32 binding)
{
	return gpu_descriptor_write_image(writer, DESCRIPTOR_TYPE_STORAGE_IMAGE, image, binding);
}

fn_inline gpu_descriptor_write_t
gpu_descriptor_write_sampled_image(struct gpu_descriptor_write_t *writer, struct gpu_image_t *image, u32 binding)
{
	return gpu_descriptor_write_image(writer, DESCRIPTOR_TYPE_SAMPLED_IMAGE, image, binding);
}

fn_inline gpu_descriptor_write_t 
gpu_descriptor_write_image_sampler(struct gpu_descriptor_write_t *writer, struct gpu_image_t *image, u32 binding)
{
	return gpu_descriptor_write_image(writer, DESCRIPTOR_TYPE_SAMPLER, image, binding);
}

//
// Gpu Device
// 

typedef struct {
    u32                       software_version;
    const char               *software_name;
    struct os_window_surface *native_surface;
} gpu_device_info_t;

fn_export void gpu_device_create(gpu_device_info_t *device_info, struct gpu_device_t *device, int *device_size);
fn_export void gpu_device_destroy(struct gpu_device_t *device);
fn_export void gpu_device_on_resize(struct gpu_device_t *self, u32 width, u32 height);
fn_export void gpu_device_begin_frame(struct gpu_device_t *self);
fn_export void gpu_device_end_frame(struct gpu_device_t *self);

fn_export struct gpu_descriptor_layout_t* gpu_device_build_descriptor_layout(struct gpu_device_t *self, gpu_descriptor_layout_flags_t type, gpu_shader_stage_flags_t flags,
	gpu_descriptor_binding_t *bindings, u32 bindings_count);
fn_export void gpu_device_destroy_descriptor_layout(struct gpu_device_t *self, struct gpu_descriptor_layout_t *layout);

fn_export struct gpu_descriptor_pool_t* gpu_device_create_descriptor_pool(
	struct gpu_device_t              *self, 
	gpu_descriptor_pool_flags_t        flags, 
	gpu_descriptor_pool_size_ratio_t *descriptor_ratios,
	u32                               descriptor_ratio_count,
	u32                               max_descriptor_sets);

fn_export void gpu_device_destroy_descriptor_pool(struct gpu_device_t *self, struct gpu_descriptor_pool_t *pool);
fn_export void gpu_device_reset_descriptor_pool(struct gpu_device_t *self, struct gpu_descriptor_pool_t *pool);

fn_export struct gpu_paged_descriptor_pool_t* gpu_device_create_paged_descriptor_pool(
	struct gpu_device_t              *self, 
	gpu_descriptor_pool_size_ratio_t *descriptor_ratios,
	u32                               descriptor_ratio_count,
	u32                               descriptor_set_per_pool);

fn_export void gpu_device_destroy_paged_descriptor_pool(struct gpu_device_t *self, struct gpu_paged_descriptor_pool_t *descriptor_pool);

fn_export struct gpu_descriptor_set_t* gpu_device_allocate_descriptor_sets(
	struct gpu_device_t                        *self, 
	struct gpu_descriptor_layout_t             *layout, 
	struct gpu_paged_descriptor_pool_t *descriptor_pool);

fn_export void gpu_device_update_descriptor_set(struct gpu_device_t *self, struct gpu_descriptor_set_t* descriptor_set, struct gpu_descriptor_write_t *descriptor_writes, u32 writes_count);


#endif //_GPU_DEVICE_H_
