#ifndef _VULKAN_TYPES_H_
#define _VULKAN_TYPES_H_

#include "vk.h"

#include <std/types.h>
#include <std/cassert.h>
#include <render/gpu_device.h>

struct allocator_t;

#define GPU_ENABLE_DEBUG_LAYER 1

#define c_max_buffered_frames 3

typedef struct gpu_descriptor_pool_t
{
	gpu_descriptor_pool_flags_t     flags;
	VkDescriptorPool                handle;
} vulkan_descriptor_pool_t;

typedef struct gpu_paged_descriptor_pool_t
{
	gpu_descriptor_pool_size_ratio_t ratios[DESCRIPTOR_TYPE_COUNT];
	u32                              ratios_count;
	u32                              descriptors_per_pool;

	VkDescriptorPool                 full_pools[100];
	u32                              full_pools_count;
	VkDescriptorPool                 ready_pools[100];
	u32                              ready_pools_count;
} vulkan_paged_descriptor_pool_t;

typedef struct allocated_image_t {
    VkImage       image;
    VkImageView   view;
    VmaAllocation memory;
    VkExtent3D    dims;
    VkFormat      format;
} allocated_image_t;

typedef struct allocated_buffer_t {
    VkBuffer          buffer;
    VmaAllocation     memory;
    VmaAllocationInfo info;
} allocated_buffer_t;

typedef struct gpu_image_t {
	allocated_image_t vma_image;
} vulkan_image_t;

typedef struct gpu_buffer_t {
	allocated_buffer_t vma_buffer;
} vulkan_buffer_t;

typedef enum
{
    CBS_CLOSED,
    CBS_OPEN,
    CBS_RESET,
} vulkan_command_buffer_state_t;

typedef struct vulkan_command_buffer_t
{
    VkCommandBuffer               handle;
    vulkan_command_buffer_state_t state;
    VkPipeline                    bound_pipeline;
} vulkan_command_buffer_t;

typedef struct vulkan_present_context_t
{
    u32                     queue_index;
    VkQueue                 queue;
    VkCommandPool           command_pool;
    u32                     frame_index;
    vulkan_command_buffer_t command_buffers[c_max_buffered_frames];
    VkSemaphore             present_semaphores[c_max_buffered_frames];
    VkSemaphore             render_semaphores[c_max_buffered_frames];
    VkFence                 render_fences[c_max_buffered_frames];
} vulkan_present_context_t;

typedef struct vulkan_graphics_context_t
{
    VkQueue                 queue;
    VkCommandPool           command_pool;
    u32                     frame_index;
    VkSemaphore             timeline_semaphore;
    u64                     semaphore_value;

    vulkan_command_buffer_t command_buffers[c_max_buffered_frames];
    u64                     fence_values[c_max_buffered_frames];
} vulkan_graphics_context_t;

typedef struct
{
    VkSurfaceCapabilitiesKHR  capabilities;
    VkSurfaceFormatKHR       *formats;
    u32                       formats_count;
    VkPresentModeKHR         *present_modes;
    u32                       present_modes_count;
} swapchain_support_info_t;

typedef struct vulkan_swapchain_t
{
    VkSwapchainKHR handle;

    // swapchain images
    VkImageView    image_views[c_max_buffered_frames];
    VkImage        images[c_max_buffered_frames];
    u32            images_count;
    u32            swapchain_index;

    // swapchain size
    u32            cached_width;
    u32            cached_height;
    u64            known_generation;
    u64            current_generation;
} vulkan_swapchain_t;

typedef u32 queue_family_t;
static const queue_family_t c_invalid_queue_family = (queue_family_t)-1;

typedef struct {
    queue_family_t present;
    queue_family_t graphics;
    queue_family_t compute;
    queue_family_t transfer;
} vulkan_queue_families_t;

typedef struct {
    VkPhysicalDevice                 handle;
    swapchain_support_info_t         swapchain_info;
    VkPhysicalDeviceProperties       properties;
    VkPhysicalDeviceFeatures         features;
    VkPhysicalDeviceMemoryProperties memory_properties;
    vulkan_queue_families_t          queues;
    bool                             supports_device_local_host_visible;
    // supported swapchain surface format
    VkSurfaceFormatKHR               surface_format;
    // supported swapchain depth format
    VkFormat                         depth_format;
} vulkan_gpu_t;

typedef struct gpu_device_t {
    VkInstance               instance;
    VkSurfaceKHR             surface;
    vulkan_gpu_t             gpu;
    VkDevice                 handle;

    VkQueue                  present_queue;
    VkQueue                  graphics_queue;
    VkQueue                  compute_queue;
    VkQueue                  transfer_queue;

    vulkan_swapchain_t       swapchain;

    VmaAllocator             vma_allocator;

    vulkan_present_context_t  present_context;
    vulkan_graphics_context_t graphics_context;

	struct allocator_t        *gpu_buffer_allocator;
	struct allocator_t        *gpu_image_allocator;
	struct allocator_t        *gpu_descriptor_pool_allocator;
	struct allocator_t        *paged_descriptor_pool_allocator;

#if GPU_ENABLE_DEBUG_LAYER
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
} vulkan_device_t;


//
// Helper Functions
//

fn_inline VkExtent3D
swapchain_get_extent(vulkan_swapchain_t *self)
{
    VkExtent3D result = {};
    result.width  = self->cached_width;
    result.height = self->cached_height;
    result.depth  = 1;
    return result;
}

fn_inline VkImage
get_swapchain_image(vulkan_swapchain_t *self)
{
    return self->images[self->swapchain_index];
}

fn_inline VkImageView
get_swapchain_image_view(vulkan_swapchain_t *self)
{
    return self->image_views[self->swapchain_index];
}

fn_inline void
swapchain_invalidate(vulkan_swapchain_t *self)
{
    self->current_generation++;
}

#endif //_VULKAN_TYPES_H_
