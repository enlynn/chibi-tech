#ifndef _VULKAN_DEVICE_H_
#define _VULKAN_DEVICE_H_

#include "vk.h"

struct gpu_device_t;

VkSemaphore create_semaphore(struct gpu_device_t *self);
VkSemaphore create_timeline_semaphore(struct gpu_device_t *self, u64 initial_value);
void destroy_semaphore(struct gpu_device_t *self, VkSemaphore semaphore);

VkFence create_fence(struct gpu_device_t *self, bool set_signaled);
void destroy_fence(struct gpu_device_t *self, VkFence fence);

#endif //_VULKAN_DEVICE_H_
