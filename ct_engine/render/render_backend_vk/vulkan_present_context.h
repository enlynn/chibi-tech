#ifndef _VULKAN_PRESENT_CONTEXT_H_
#define _VULKAN_PRESENT_CONTEXT_H_

#include "vk.h"

struct gpu_device_t;
struct vulkan_present_context_t;
struct vulkan_graphics_context_t;
struct vulkan_swapchain_t;
struct allocated_image_t;

void vulkan_present_context_init(struct vulkan_present_context_t *self, struct gpu_device_t *device);
void vulkan_present_context_deinit(struct vulkan_present_context_t *self, struct gpu_device_t *device);
void vulkan_present_context_present(struct vulkan_present_context_t *self, struct vulkan_graphics_context_t *graphics_context,
    struct gpu_device_t       *device,
    struct vulkan_swapchain_t *swapchain,
    struct allocated_image_t  *presented_image);

#endif //_VULKAN_PRESENT_CONTEXT_H_
